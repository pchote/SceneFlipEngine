/*
 * This file is part of SceneFlipEngine.
 * Copyright 2012, 2017 Paul Chote
 *
 * SceneFlipEngine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SceneFlipEngine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SceneFlipEngine.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "luabridge.h"
#include "luabridge_scene.h"
#include "engine.h"
#include "renderer.h"
#include "framebuffer.h"
#include "modelview.h"
#include "matrix.h"
#include "scene.h"
#include "layer.h"
#include "walkmap.h"
#include "actor.h"

struct actor_list
{
    actor_ptr actor;
    struct actor_list *next;
};

struct layer_list
{
    layer_ptr layer;
    struct layer_list *next;
};

struct timeout_list
{
    GLfloat ms_remaining;
    int callback;

    struct timeout_list *next;
};

struct scene
{
    lua_State *lua;
    struct camera_state camera;

    struct layer_list *layers;
    walkmap_ptr walkmap;

    // Camera matrices
    modelview_ptr mv;

    // Actors
    struct actor_list *actors;
    struct actor_list **actors_tail;

    // Tiemout callbacks
    struct timeout_list *timeouts;
    struct timeout_list **timeouts_tail;

    // OpenGL state
    framebuffer_ptr fb;

    // Cached texture rectangle size
    GLuint width;
    GLuint height;
};

/*
 * Create a scene
 *
 * Call Context: Worker thread
 */
scene_ptr scene_create(const char *scene_prefix, GLuint width, GLuint height, engine_ptr e)
{
    // Initialize scene
    scene_ptr s = calloc(1, sizeof(struct scene));
    assert(s);

    // Load scene metadata
    char *scene_path = calloc(strlen(scene_prefix) + 17, sizeof(char));
    assert(scene_path);
    sprintf(scene_path, "scenes/%s/scene.lua", scene_prefix);
    s->lua = luabridge_load(scene_path);

    sprintf(scene_path, "scenes/%s/scene.map", scene_prefix);
    s->walkmap = walkmap_create(scene_path, e);
    free(scene_path);

    // Prepare camera and framebuffer
    GLfloat projection[16];
    s->width = width;
    s->height = height;
    s->mv = modelview_create();
    luabridge_parse_scene_camera(s->lua, &s->camera);
    s->camera.aspect = width*1.0f/height;

    mtxLoadPerspective(projection, s->camera.fov, s->camera.aspect,
                       s->camera.z_near, s->camera.z_far);
    modelview_set_projection(s->mv, projection);
    scene_update_camera(s, (GPpolar){0,0});

    s->actors_tail = &s->actors;
    s->timeouts_tail = &s->timeouts;

    // Run setup script
    luabridge_set_globals(s->lua, s, s->walkmap, e, true);
    luabridge_run_setup(s->lua, s);
    luabridge_clear_globals(s->lua);

    // Init framebuffer
    s->fb = framebuffer_create(s->width, s->height, e);

    // Block until all previous tasks have completed
    engine_synchronize_tasks(e);
    return s;
}

/*
 * Destroy scene state
 *
 * Call Context: Main thread
 */
void scene_destroy(scene_ptr s, engine_ptr e)
{
    for (struct layer_list *ll = s->layers, *next; ll; ll = next)
    {
        layer_destroy(ll->layer, e);

        next = ll->next;
        free(ll);
    }

    for (struct actor_list *al = s->actors, *next; al; al = next)
    {
        actor_destroy(al->actor, s->walkmap, e);

        next = al->next;
        free(al);
    }

    for (struct timeout_list *tl = s->timeouts, *next; tl; tl = next)
    {
        // Remove callback from lua registry
        luabridge_destroy_ref(s->lua, tl->callback);

        next = tl->next;
        free(tl);
    }

    walkmap_destroy(s->walkmap, e);

    modelview_destroy(s->mv);
    lua_close(s->lua);

    framebuffer_destroy(s->fb, e);
    free(s);
}

/*
 * Render scene into its framebuffer and return
 * a reference to the texture
 *
 * Call Context: Main thread
 *
 * Renders in a specific order to avoid rendering artefacts
 * with translucent pixels:
 *  - First renderer all actors
 *  - Then, render layers with ascending y coordinate
 *    (layers are sorted during scene creation)
 *  - Then overlay any debug information
 *
 * Assumptions:
 *  - Actors are opaque
 *  - Layers with translucent regions do not intersect any other layers
 *    (so that the z-sorting remains correct, otherwise fragments may be lost)
 */

textureref scene_draw(scene_ptr s, engine_config_ptr ec, renderer_ptr r)
{
    assert(s);

    framebuffer_bind(s->fb);
    for (struct actor_list *al = s->actors; al; al = al->next)
        actor_draw(al->actor, s->mv, r);

    for (struct layer_list *ll = s->layers; ll; ll = ll->next)
        layer_draw(ll->layer, s->mv, r);

    glDisable(GL_DEPTH_TEST);
    if (ec->debug_render_layer_mesh)
        for (struct layer_list *ll = s->layers; ll; ll = ll->next)
            layer_debug_draw(ll->layer, s->mv, r);

    if (ec->debug_render_walkmesh)
        walkmap_debug_draw_walkmesh(s->walkmap, s->mv, r);

    if (ec->debug_render_collisions)
        walkmap_debug_draw_collisions(s->walkmap, s->mv, r);
    glEnable(GL_DEPTH_TEST);

    framebuffer_unbind(s->fb);
    return framebuffer_get_textureref(s->fb);
}


static void trigger_zone_callback(scene_ptr s, actor_ptr a, int callback)
{
    luabridge_scene_run_trigger(s->lua, a, callback);
}

/*
 * Update the scene state
 */
void scene_tick(scene_ptr s, engine_ptr e, double dt)
{
    luabridge_set_globals(s->lua, s, s->walkmap, e, false);

    // Tick timed function callbacks
    GLfloat ms = (GLfloat)dt*1000;
    struct timeout_list **ptl = &s->timeouts;
    while (*ptl)
    {
        struct timeout_list *tl = *ptl;
        tl->ms_remaining -= ms;

        if (tl->ms_remaining > 0)
        {
            // Advance to next item
            ptl = &(*ptl)->next;
            continue;
        }

        // Run timeout function and remove from list
        luabridge_scene_run_timeout(s->lua, tl->callback, -tl->ms_remaining);
        *ptl = tl->next;

        // Remove callback from lua registry
        luabridge_destroy_ref(s->lua, tl->callback);
        free(tl);
    }

    walkmap_tick(s->walkmap, dt);
    walkmap_check_triggers(s->walkmap, s, trigger_zone_callback);
    luabridge_run_tick(s->lua, s, e, dt);
    luabridge_clear_globals(s->lua);
}

/*
 * Fetch a copy of the camera_state struct
 */
struct camera_state scene_camera(scene_ptr s)
{
    return s->camera;
}

/*
 * Set the debug camera offset
 */
void scene_update_camera(scene_ptr s, GPpolar offset)
{
    s->camera.debug_offset = offset;

    // Position camera
    GLfloat camera[16];
    mtxLoadIdentity(camera);
    mtxTranslateApply(camera, 0, 0, -s->camera.debug_offset.radius);
    mtxRotateXApply(camera, -(s->camera.pitch + 90));
    mtxRotateZApply(camera, s->camera.yaw);
    mtxTranslateApply(camera, -s->camera.pos[0], -s->camera.pos[1], -s->camera.pos[2]);
    mtxRotateZApply(camera, s->camera.debug_offset.angle);

    modelview_set_camera(s->mv, camera);
}

actor_ptr scene_load_actor(scene_ptr s, const char *model, GLfloat collision_radius, engine_ptr e)
{
    struct actor_list *al = calloc(1, sizeof(struct actor_list));
    assert(al);

    al->actor = actor_create(model, collision_radius, s->walkmap, e);
    assert(al->actor);

    // Append to tail of list
    *s->actors_tail = al;
    s->actors_tail = &al->next;

    return al->actor;
}

layer_ptr scene_load_layer(scene_ptr s, const char *image, GLfloat *screen_region, GLfloat depth,
                           GLfloat *frame_regions, GLsizei frame_count, GLfloat *normal, engine_ptr e)
{
    struct layer_list *ll = calloc(1, sizeof(struct layer_list));
    assert(ll);

    ll->layer = layer_create(image, screen_region, depth, frame_regions, frame_count, normal, &s->camera, e);
    assert(ll->layer);

    // Sort layers into the correct render order on insert
    GLfloat order = layer_render_order(ll->layer);
    if (!s->layers)
    {
        // Empty list
        s->layers = ll;
        return ll->layer;
    }

    struct layer_list *cur = s->layers;
    GLfloat cur_order = layer_render_order(cur->layer);
    if (order > cur_order)
    {
        // Backmost layer - insert at start
        ll->next = s->layers;
        s->layers = ll;
        return ll->layer;
    }

    while (cur->next)
    {
        GLfloat next_order = layer_render_order(cur->next->layer);
        if (order <= cur_order && order >= next_order);
        {
            // Intermediate layer
            ll->next = cur->next;
            cur->next = ll;
            return ll->layer;
        }
        cur = cur->next;
        cur_order = next_order;
    }

    // Frontmost layer - insert at end
    cur->next = ll;

    return ll->layer;
}

void scene_add_trigger_region(scene_ptr s, GLfloat pos[3], GLfloat *vertices, GLsizei vertex_count, int callback, engine_ptr e)
{
    walkmap_register_trigger_region(s->walkmap, pos, vertices, vertex_count, callback, e);
}

void scene_add_timeout(scene_ptr s, int callback, GLfloat ms)
{
    struct timeout_list *tl = calloc(1, sizeof(struct timeout_list));
    assert(tl);

    tl->ms_remaining = ms;
    tl->callback = callback;

    *s->timeouts_tail = tl;
    s->timeouts_tail = &tl->next;
}

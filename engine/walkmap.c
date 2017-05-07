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
#include <math.h>
#include "engine.h"
#include "renderer.h"
#include "matrix.h"
#include "modelview.h"
#include "vertexarray.h"
#include "scene.h"
#include "walkmap.h"
#include "actor.h"
#include "collision.h"

/*
 * Private implementation details
 */

GLfloat *group_colors[] =
{
    (GLfloat[]){1,0,1,1},
    (GLfloat[]){0,1,1,1},
    (GLfloat[]){0.1,0.1,0.5,1},
    (GLfloat[]){0,1,0,1},
    (GLfloat[]){1,1,1,1},
    (GLfloat[]){1,0,1,1},
    (GLfloat[]){1,1,0,1},
    (GLfloat[]){1,0,0,1},
    // TODO: Add 8 more unique colors
    (GLfloat[]){1,0,1,1},
    (GLfloat[]){0,1,1,1},
    (GLfloat[]){0.1,0.1,0.5,1},
    (GLfloat[]){0,1,0,1},
    (GLfloat[]){1,1,1,1},
    (GLfloat[]){1,0,1,1},
    (GLfloat[]){1,1,0,1},
    (GLfloat[]){1,0,0,1},
};

struct walkmap_triangle
{
    // Read from file
    uint16_t group;
    uint16_t group_interaction_mask;

    // Vertices
    GLfloat a[3];
    GLfloat b[3];
    GLfloat c[3];

    // Calculated at runtime
    // Vectors from c to a,b
    GLfloat ca[3];
    GLfloat cb[3];

    // 1 / determinant of transform to barycentric coords
    GLfloat invdet;

    collision_object_t co;
};

struct walkmap_border
{
    uint16_t group;
    uint16_t group_interaction_mask;
    collision_object_t co;

    // For debug display
    vertexarray_ptr border_debug;
};

struct trigger_region_list
{
    int callback;
    collision_object_t co;

    // For debug display
    vertexarray_ptr debug;
    uint16_t group;
    GLfloat position[3];

    struct trigger_region_list *next;
};

struct walkmap
{
    // Walkmap mesh
    struct walkmap_triangle *triangles;
    uint32_t triangle_count;

    struct walkmap_border *borders;
    uint32_t border_count;

    // Trigger zones are stored as a linked list
    struct trigger_region_list *triggers;
    struct trigger_region_list **triggers_tail;

    // For debug display
    vertexarray_ptr actor_debug;
    vertexarray_ptr height_debug[16];

    // For box2d
    collision_world_t walkmap_triangle_lookup;
    collision_world_t collision;
    collision_world_t trigger_lookup;
};

struct walkmap_actordata
{
    actor_ptr actor;
    GLfloat position[3];
    GLfloat radius;
    struct walkmap_triangle *current_triangle;
    void (*movement_callback)(actor_ptr, GLfloat[3], GLfloat[3]);
    collision_object_t co;
};

/*
 * Calculate the height intersection for a given point
 * with the plane defined by the given triangle
 */
static GLfloat walkmap_triangle_height_at_point(struct walkmap_triangle *t, GLfloat xy[2])
{
    // Calculate barycentric coordinates u,v
    GLfloat dx = xy[0] - t->c[0];
    GLfloat dy = xy[1] - t->c[1];
    GLfloat u =  t->invdet*(t->cb[1]*dx - t->cb[0]*dy);
    GLfloat v = -t->invdet*(t->ca[1]*dx - t->ca[0]*dy);

    // Calculate w (height) from u,v
    return u*t->a[2] + v*t->b[2] + (1 - u - v)*t->c[2];
}

static void *walkmap_triangle_hittest_height_filter(void *current, void *test, void *callbackdata)
{
    GLfloat *pos = callbackdata;

    GLfloat test_height = walkmap_triangle_height_at_point((struct walkmap_triangle *)test, pos);
    GLfloat current_height = walkmap_triangle_height_at_point((struct walkmap_triangle *)current, pos);
    return (fabs(test_height - pos[2]) < fabs(current_height - pos[2])) ? test : current;
}

static void *walkmap_triangle_hittest_group_filter(void *_current, void *_test, void *callbackdata)
{
    struct walkmap_triangle *current = _current;
    struct walkmap_triangle *test = _test;
    uint16_t group_mask = *((uint16_t *)callbackdata);

    // Matches collision group better than current match
    if ((group_mask & (1 << test->group)) &&
        !(group_mask & (1 << current->group)))
        return test;

    return current;
}

static void update_actor_data(walkmap_ptr w, walkmap_actordata_ptr ad, bool prioritize_height)
{
    GLfloat pos[2];
    collision_object_position(ad->co, pos);
    struct walkmap_triangle *wt;

    // Filter based on closest z-separation, ignoring group
    if (prioritize_height)
        wt = collision_world_hittest(w->walkmap_triangle_lookup, pos, walkmap_triangle_hittest_height_filter, ad->position);
    else
    {
        // Filter based on group mask, ignoring height
        uint16_t mask = ad->current_triangle ? ad->current_triangle->group_interaction_mask : 0xFFFF;
        wt = collision_world_hittest(w->walkmap_triangle_lookup, pos, walkmap_triangle_hittest_group_filter, &mask);
    }

    if (!wt)
    {
        fprintf(stderr, "Actor outside walkmap");
        assert(FATAL_ERROR);
    }

    // Actor collison flags are set from the current triangle
    collision_object_copy_collisiondata(ad->co, wt->co);

    // Update current triangle ref
    if (wt != ad->current_triangle)
        ad->current_triangle = wt;

    // Update position
    ad->position[0] = pos[0];
    ad->position[1] = pos[1];
    ad->position[2] = walkmap_triangle_height_at_point(ad->current_triangle, ad->position);
}

/*
 * Load mesh definition from file into the triangles array
 * and initializes the vertex array for debug display
 *
 * Call Context: Worker thread
 */
static void load_map(walkmap_ptr w, const char *map_path, engine_ptr e)
{
    /*
     * .map format:
     *
     * uint32_t version
     * uint32_t vertex_count
     * uint32_t triangle_count;
     * uint32_t border_count;
     * GLfloat vertices[3*vertex_count]; // x,y,z vertex data triplets
     * triangle_count x:
     *    uint16_t group;
     *    uint16_t group_interaction_mask;
     *    uint32_t indices[3];
     * border_count x:
     *    uint16_t group;
     *    uint16_t group_interaction_mask;
     *    uint32_t length;
     *    uint32_t indices[length];
     */
    FILE *input = fopen(map_path, "rb");
    assert(input);

    // Read static header data
    uint32_t version, vertex_count;
    assert(fread(&version, sizeof(uint32_t), 1, input) == 1);
    assert(fread(&vertex_count, sizeof(uint32_t), 1, input) == 1);
    assert(fread(&w->triangle_count, sizeof(uint32_t), 1, input) == 1);
    assert(fread(&w->border_count, sizeof(uint32_t), 1, input) == 1);

    // Read vertex data
    GLfloat *vertices = calloc(3*vertex_count, sizeof(GLfloat));
    assert(vertices);
    assert(fread(vertices, sizeof(GLfloat), 3*vertex_count, input) == 3*vertex_count);

    // Read triangle data
    w->triangles = calloc(w->triangle_count, sizeof(struct walkmap_triangle));
    assert(w->triangles);

    for (size_t i = 0; i < w->triangle_count; i++)
    {
        struct walkmap_triangle *wt = &w->triangles[i];
        assert(fread(&wt->group, sizeof(uint16_t), 1, input) == 1);
        assert(wt->group < 16);
        assert(fread(&wt->group_interaction_mask, sizeof(uint16_t), 1, input) == 1);

        uint32_t indices[3];
        assert(fread(&indices, sizeof(uint32_t), 3, input) == 3);
        memcpy(wt->a, &vertices[3*indices[0]], 3*sizeof(GLfloat));
        memcpy(wt->b, &vertices[3*indices[1]], 3*sizeof(GLfloat));
        memcpy(wt->c, &vertices[3*indices[2]], 3*sizeof(GLfloat));

        // Cache intermediate quantities for calculating
        // barycentric coordinates for height calculation
        wt->ca[0] = wt->a[0] - wt->c[0];
        wt->ca[1] = wt->a[1] - wt->c[1];
        wt->cb[0] = wt->b[0] - wt->c[0];
        wt->cb[1] = wt->b[1] - wt->c[1];
        wt->invdet = 1.0/(wt->cb[1]*wt->ca[0] - wt->cb[0]*wt->ca[1]);
        wt->co = collision_object_create_triangle(w->walkmap_triangle_lookup, wt->a, wt->b, wt->c,
                                                  wt->group, wt->group_interaction_mask, wt);
    }

    // Generate debug mesh vertex arrays
    for (size_t j = 0; j < 16; j++)
    {
        uint32_t count = 0;
        for (size_t i = 0; i < w->triangle_count; i++)
            if (w->triangles[i].group == j)
                count++;

        if (count == 0)
            continue;

        GLfloat *mesh_vertices = calloc(9*count, sizeof(GLfloat));
        assert(mesh_vertices);

        for (size_t k = 0, i = 0; i < w->triangle_count; i++)
            if (w->triangles[i].group == j)
            {
                struct walkmap_triangle *wt = &w->triangles[i];
                memcpy(&mesh_vertices[9*k], wt->a, 3*sizeof(GLfloat));
                memcpy(&mesh_vertices[9*k+3], wt->b, 3*sizeof(GLfloat));
                memcpy(&mesh_vertices[9*k+6], wt->c, 3*sizeof(GLfloat));
                k++;
            }

        w->height_debug[j] = vertexarray_create(mesh_vertices, NULL, 3*count, 0, GL_TRIANGLES, e);
        free(mesh_vertices);
    }

    // Read border data
    w->borders = calloc(w->border_count, sizeof(struct walkmap_border));
    assert(w->borders);

    for (size_t j = 0; j < w->border_count; j++)
    {
        struct walkmap_border *wb = &w->borders[j];
        assert(fread(&wb->group, sizeof(uint16_t), 1, input) == 1);
        assert(wb->group < 16);
        assert(fread(&wb->group_interaction_mask, sizeof(uint16_t), 1, input) == 1);

        GLsizei length;
        assert(fread(&length, sizeof(uint32_t), 1, input) == 1);

        GLfloat *border_vertices = calloc(3*length, sizeof(GLfloat));
        assert(border_vertices);

        for (GLuint i = 0; i < length; i++)
        {
            uint32_t index;
            assert(fread(&index, sizeof(uint32_t), 1, input) == 1);
            memcpy(&border_vertices[3*i], &vertices[3*index], 3*sizeof(GLfloat));
        }

        wb->co = collision_object_create_chain(w->collision, border_vertices, length,
                                               wb->group, wb->group_interaction_mask, NULL);
        wb->border_debug = vertexarray_create(border_vertices, NULL, length, 0, GL_LINE_STRIP, e);
    }

    free(vertices);
    fclose(input);

    GLfloat circle_vertices[48];
    for (size_t i = 0; i < 16; i++)
    {
        circle_vertices[3*i]   = sin(i*M_PI/7.5);
        circle_vertices[3*i+1] = cos(i*M_PI/7.5);
        circle_vertices[3*i+2] = 0;
    }
    w->actor_debug = vertexarray_create(circle_vertices, NULL, 16, 0, GL_LINE_STRIP, e);
}

#pragma mark Public Interface
/*
 * Create a walkmap
 *
 * Call Context: Worker thread
 */
walkmap_ptr walkmap_create(const char *map_path, engine_ptr e)
{
    walkmap_ptr w = calloc(1, sizeof(struct walkmap));
    assert(w);

    // We don't use gravity
    w->walkmap_triangle_lookup = collision_world_create();
    w->collision = collision_world_create();
    w->trigger_lookup = collision_world_create();
    w->triggers_tail = &w->triggers;

    // Temporary hack: hardcode map definition
    load_map(w, map_path, e);

    return w;
}

/*
 * Release resources associated with this layer
 */
void walkmap_destroy(walkmap_ptr w, engine_ptr e)
{
    for (size_t i = 0; i < w->border_count; i++)
    {
        collision_object_free(w->borders[i].co, w->collision);
        vertexarray_destroy(w->borders[i].border_debug, e);
    }

    assert(collision_world_count(w->collision) == 0);
    collision_world_free(w->collision);

    for (size_t i = 0; i < w->triangle_count; i++)
        collision_object_free(w->triangles[i].co, w->walkmap_triangle_lookup);

    free(w->triangles);

    assert(collision_world_count(w->walkmap_triangle_lookup) == 0);
    collision_world_free(w->walkmap_triangle_lookup);

    for (struct trigger_region_list *tr = w->triggers, *next; tr; tr = next)
    {
        collision_object_free(tr->co, w->trigger_lookup);
        next = tr->next;
        free(tr);
    }

    assert(collision_world_count(w->trigger_lookup) == 0);
    collision_world_free(w->trigger_lookup);

    for (size_t i = 0; i < 16; i++)
        if (w->height_debug[i])
            vertexarray_destroy(w->height_debug[i], e);
    free(w);
}

/*
 * Render layer into the current gl context
 *
 * Call Context: Main thread
 */
void walkmap_debug_draw_collisions(walkmap_ptr w, modelview_ptr mv, renderer_ptr r)
{
#if !PLATFORM_GLES
    GLfloat mvp[16];
    modelview_push(mv);
    modelview_calculate_mvp(mv, mvp);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); checkGLError();

    // Draw borders
    for (GLsizei i = 0; i < w->border_count; i++)
    {
        renderer_enable_line_shader(r, mvp, group_colors[w->borders[i].group]);
        vertexarray_draw(w->borders[i].border_debug);
    }

    // Trigger regions
    for (struct trigger_region_list *tr = w->triggers; tr; tr = tr->next)
    {
        GLfloat *modelview = modelview_push(mv);
        mtxTranslateApply(modelview, tr->position[0], tr->position[1], tr->position[2]);
        modelview_calculate_mvp(mv, mvp);
        renderer_enable_line_shader(r, mvp, group_colors[tr->group]);
        vertexarray_draw(tr->debug);
        modelview_pop(mv);
    }
    
    // Actors
    collision_iterator_t it = collision_iterator_create(w->collision);
    while (!collision_iterator_finished(it))
    {
        // Only actors have userdata set
        walkmap_actordata_ptr ad = collision_iterator_userdata(it);
        if (ad != NULL)
        {
            GLfloat *modelview = modelview_push(mv);
            mtxTranslateApply(modelview, ad->position[0], ad->position[1], ad->position[2]);
            mtxScaleApply(modelview, ad->radius, ad->radius, ad->radius);

            // Outer circle gives the group that the center of the actor is in
            // (i.e. used for height calculations)
            modelview_calculate_mvp(mv, mvp);
            uint8_t current_group = ad->current_triangle->group;
            renderer_enable_line_shader(r, mvp, group_colors[current_group]);
            vertexarray_draw(w->actor_debug);

            // Draw smaller circles for each group that the actor considers for collisions
            uint16_t group_interaction_mask = collision_object_collision_mask(ad->co);
            for (uint8_t i = 0; i < 16; i++)
            {
                if (i == current_group)
                    continue;

                if (group_interaction_mask & (1 << i))
                {
                    mtxScaleApply(modelview, 0.9, 0.9, 0.9);
                    modelview_calculate_mvp(mv, mvp);
                    renderer_enable_line_shader(r, mvp, group_colors[i]);
                    vertexarray_draw(w->actor_debug);
                }
            }

            modelview_pop(mv);
        }
        collision_iterator_advance(it);
    }
    collision_iterator_free(it);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); checkGLError();
    modelview_pop(mv);
#endif
}

void walkmap_debug_draw_walkmesh(walkmap_ptr w, modelview_ptr mv, renderer_ptr r)
{
#if !PLATFORM_GLES
    GLfloat mvp[16];
    modelview_push(mv);
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_line_shader(r, mvp, group_colors[0]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); checkGLError();

    // Draw walk mesh
    for (size_t i = 0; i < 16; i++)
        if (w->height_debug[i])
        {
            renderer_enable_line_shader(r, mvp, group_colors[i]);
            vertexarray_draw(w->height_debug[i]);
        }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); checkGLError();
    modelview_pop(mv);
#endif
}

void walkmap_tick(walkmap_ptr w, double dt)
{
    collision_world_tick(w->collision, dt);
    collision_iterator_t it = collision_iterator_create(w->collision);
    while (!collision_iterator_finished(it))
    {
        // Only actors have userdata set
        walkmap_actordata_ptr ad = collision_iterator_userdata(it);
        if (ad != NULL)
        {
            if (ad->movement_callback)
            {
                GLfloat old_pos[3];
                old_pos[0] = ad->position[0];
                old_pos[1] = ad->position[1];
                old_pos[2] = ad->position[2];
                update_actor_data(w, ad, false);
                ad->movement_callback(ad->actor, ad->position, old_pos);
            }
            else
                update_actor_data(w, ad, false);
        }
        collision_iterator_advance(it);
    }
    collision_iterator_free(it);
}

void walkmap_check_triggers(walkmap_ptr w, scene_ptr s, void (*trigger_callback)(scene_ptr, actor_ptr, int))
{
    collision_iterator_t it = collision_iterator_create(w->collision);
    while (!collision_iterator_finished(it))
    {
        // Only actors have userdata set
        walkmap_actordata_ptr ad = collision_iterator_userdata(it);
        if (ad != NULL)
            for (struct trigger_region_list *tr = w->triggers; tr; tr = tr->next)
                if (collision_object_hittest(tr->co, ad->position, 1 << ad->current_triangle->group))
                    trigger_callback(s, ad->actor, tr->callback);

        collision_iterator_advance(it);
    }
    collision_iterator_free(it);
}

walkmap_actordata_ptr walkmap_register_actor(walkmap_ptr w, actor_ptr a, GLfloat pos[3], GLfloat radius)
{
    walkmap_actordata_ptr ad = calloc(1, sizeof(struct walkmap_actordata));
    ad->actor = a;
    ad->movement_callback = NULL;
    ad->radius = radius;
    ad->position[2] = pos[2];
    assert(ad);

    ad->co = collision_object_create_circle(w->collision, pos, radius, ad);
    update_actor_data(w, ad, true);
    return ad;
}

void walkmap_unregister_actor(walkmap_ptr w, walkmap_actordata_ptr ad)
{
    collision_object_free(ad->co, w->collision);
    printf("Unregistered actor %p\n", ad->actor);
}

void walkmap_set_movement_callback(walkmap_ptr w, walkmap_actordata_ptr ad, void (*movement_callback)(actor_ptr, GLfloat[3], GLfloat[3]))
{
    ad->movement_callback = movement_callback;
}

void walkmap_actor_position(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat p[3])
{
    p[0] = ad->position[0];
    p[1] = ad->position[1];
    p[2] = ad->position[2];
}

void walkmap_set_actor_position(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat p[3])
{
    collision_object_set_position(ad->co, p);
    ad->position[2] = p[2];
    update_actor_data(w, ad, true);
}

void walkmap_actor_velocity(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat v[2])
{
    collision_object_velocity(ad->co, v);
}

void walkmap_set_actor_velocity(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat v[2])
{
    collision_object_set_velocity(ad->co, v);
}

void walkmap_register_trigger_region(walkmap_ptr w, GLfloat pos[3], GLfloat *vertices, GLsizei vertex_count, int callback, engine_ptr e)
{
    struct trigger_region_list *tr = calloc(1, sizeof(struct trigger_region_list));
    assert(tr);
    tr->callback = callback;
    memcpy(tr->position, pos, 3*sizeof(GLfloat));

    // Filter based on closest z-separation, ignoring group
    struct walkmap_triangle *wt = collision_world_hittest(w->walkmap_triangle_lookup, pos, walkmap_triangle_hittest_height_filter, pos);
    if (!wt)
    {
        fprintf(stderr, "Trigger zone outside walkmap");
        assert(FATAL_ERROR);
    }

    GLsizei debug_vertex_count = vertex_count + 1;
    GLfloat *debug_vertices = calloc(3*debug_vertex_count, sizeof(GLfloat));
    assert(debug_vertices);
    GLfloat z = (wt->a[2] + wt->b[2] + wt->c[2])/3;
    for (GLsizei i = 0; i < vertex_count; i++)
    {
        debug_vertices[3*i] = vertices[2*i];
        debug_vertices[3*i+1] = vertices[2*i+1];
        debug_vertices[3*i+2] = z;
    }
    debug_vertices[3*vertex_count] = vertices[0];
    debug_vertices[3*vertex_count+1] = vertices[1];
    debug_vertices[3*vertex_count+2] = z;

    tr->group = wt->group;
    tr->co = collision_object_create_polygon(w->trigger_lookup, vertices, vertex_count, wt->group, wt->group_interaction_mask, tr);
    collision_object_set_position(tr->co, pos);
    tr->debug = vertexarray_create(debug_vertices, NULL, debug_vertex_count, 0, GL_LINE_STRIP, e);

    *w->triggers_tail = tr;
    w->triggers_tail = &tr->next;
}

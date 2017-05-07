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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "engine.h"
#include "frame.h"
#include "renderer.h"
#include "modelview.h"
#include "texture.h"
#include "framebuffer.h"
#include "scene.h"
#include "transition_instance.h"
#include "vertexarray.h"
#include "font.h"
#include "widget.h"
#include "widget_string.h"

/*
 * Private implementation details
 */
struct frame
{
    texture_ptr loadscreen;
    modelview_ptr mv;

    widget_ptr widget_root;

    // Debug info display (Frame times, FPS, etc)
    font_ptr debug_font;
    widget_string_ptr debug_metrics;
    widget_string_ptr debug_overlays;

    // Scene / fullscreen frame resolution
    GLuint width;
    GLuint height;
    vertexarray_ptr quad;

    scene_ptr current_scene;
    textureref current_textureref;
    scene_ptr next_scene;
    textureref next_textureref;

    transition_instance_ptr transition;
};

/*
 * Create a view at engine init
 *
 * Call Context: Main thread
 */
frame_ptr frame_create(const char *start_scene, GLuint width, GLuint height, engine_ptr e, renderer_ptr r)
{
    frame_ptr f = calloc(1, sizeof(struct frame));
    if (!f)
        return NULL;

    f->width = width;
    f->height = height;

    f->mv = modelview_create();
    f->loadscreen = texture_create("loadscreen.png", e);
    assert(f->loadscreen);

    f->current_scene = NULL;
    f->current_textureref = texture_get_textureref(f->loadscreen, 1, 1);

    // Scale the texture coordinates by the internal framebuffer size
    // TODO: This is a bit crap, breaking encapsulation
    GLfloat size = framebuffer_size(width, height);
    f->quad = vertexarray_create_quad(width/size, height/size, e);

    f->widget_root = widget_create_root();

    // Debug info display (Frame times, FPS, etc)
    engine_load_font(e, "debug", "Inconsolata.otf", 18);

    widget_string_ptr debug_controls = widget_string_create("debug", e);
    widget_add(f->widget_root, "debug_controls", (GLfloat[]){-1.28, -0.8}, WIDGET_STRING, debug_controls);
    widget_string_set_text(debug_controls,
                           "          \\c[#00CCFFFF]Controls\n"
                           "\\c[#FFCC00FF]w,a,s,d\\c[#FFFFFFFF]: Move player        \n"
                           "\\c[#FFCC00FF]    j,l\\c[#FFFFFFFF]: Rotate debug camera\n"
                           "\\c[#FFCC00FF]    i,k\\c[#FFFFFFFF]: Zoom debug camera  \n"
                           "\\c[#FFCC00FF]      u\\c[#FFFFFFFF]: Reset debug camera \n",
                           GL_STATIC_DRAW);

    f->debug_metrics = widget_string_create("debug", e);
    widget_add(f->widget_root, "debug_text", (GLfloat[]){-1.28, 0.94}, WIDGET_STRING, f->debug_metrics);

    f->debug_overlays = widget_string_create("debug", e);
    widget_add(f->widget_root, "debug_text", (GLfloat[]){0.8, -0.8}, WIDGET_STRING, f->debug_overlays);

    frame_update_overlay_display(f, engine_get_config_ref(e));
    frame_load_scene(f, start_scene, "startup", e, r);
    return f;
}

void frame_update_overlay_display(frame_ptr f, engine_config_ptr ec)
{
    const char *title = "\\c[#00CCFFFF]";
    const char *key = "\\c[#FFCC00FF]";
    const char *text = "\\c[#FFFFFFFF]";

    const char *on =  "\\c[#00EE00FF]ON \\c[#FFFFFFFF]";
    const char *off = "\\c[#EE0000FF]OFF\\c[#FFFFFFFF]";

    char buf[1024];
    snprintf(buf, 1024,
        "          %sOverlays%s         \n"
        "%so%s: Layer outlines     > %s\n"
        "%sp%s: Walkmap geometry   > %s\n"
        "%s[%s: Collision geometry > %s\n"
        "%s]%s: Text triangles     > %s\n",
        title, text,
        key, text, ec->debug_render_layer_mesh ? on : off,
        key, text, ec->debug_render_walkmesh ? on : off,
        key, text, ec->debug_render_collisions ? on : off,
        key, text, ec->debug_text_triangles ? on : off);

    widget_string_set_text(f->debug_overlays, buf, GL_DYNAMIC_DRAW);
}

/*
 * Destroy frame
 *
 * Call Context: Main thread
 */
void frame_destroy(frame_ptr f, engine_ptr e)
{
    modelview_destroy(f->mv);
    if (f->transition)
        transition_instance_destroy(f->transition, e);

    if (f->current_scene)
        scene_destroy(f->current_scene, e);

    if (f->next_scene)
        scene_destroy(f->next_scene, e);

    widget_destroy(f->widget_root, e);

    font_destroy(f->debug_font, e);

    free(f);
}

static void frame_transition_complete(frame_ptr f, engine_ptr e, renderer_ptr r)
{
    // current_scene will be null if this is the first scene to load
    if (f->current_scene)
        scene_destroy(f->current_scene, e);

    f->current_scene = f->next_scene;
    f->next_scene = NULL;

    transition_instance_destroy(f->transition, e);
    f->transition = NULL;

    engine_config_ptr ec = engine_get_config_ref(e);
    f->current_textureref = scene_draw(f->current_scene, ec, r);
}

/*
 * Tick the frame state
 * In practice, this is either the current scene,
 * or a transition between scenes
 *
 * Call Context: Main thread
 */
void frame_tick(frame_ptr f, double dt, engine_ptr e, renderer_ptr r)
{
    if (f->transition)
    {
        if (f->transition->type->tick(f->transition, dt, e, r))
            frame_transition_complete(f, e, r);
    }
    else
    {
        scene_tick(f->current_scene, e, dt);
        engine_config_ptr ec = engine_get_config_ref(e);
        f->current_textureref = scene_draw(f->current_scene, ec, r);
    }
}

/*
 * Create a view at engine init
 *
 * Call Context: Main thread
 */
void frame_draw(frame_ptr f, GLuint fps, GLfloat tick_time, GLfloat task_time, engine_ptr e, renderer_ptr r)
{
    GLfloat mvp[16];
    modelview_calculate_mvp(f->mv, mvp);
    glDisable(GL_DEPTH_TEST);

    if (f->transition)
    {
        f->transition->type->draw(f->transition, f->mv, r);
        glActiveTexture(GL_TEXTURE0);
    }
    else
    {        
        glBindTexture(GL_TEXTURE_2D, f->current_textureref.texture); checkGLError();
        renderer_enable_model_shader(r, mvp);
        vertexarray_draw(f->quad);
    }

    char *key = "\\c[#FFFF00FF]";
    char *text = "\\c[#FFFFFFFF]";
    char buf[1024];
    snprintf(buf, 1024,
       "  FPS: %s%4u%s\n Tick: %s%.2fms%s\nTasks: %s%.2fms%s",
       key, fps, text,
       key, tick_time*1000, text,
       key, task_time*1000, text);
    widget_string_set_text(f->debug_metrics, buf, GL_STREAM_DRAW);

    engine_config_ptr ec = engine_get_config_ref(e);
    if (ec->debug_text_triangles)
        widget_debug_draw(f->widget_root, f->mv, r);
    else
        widget_draw(f->widget_root, f->mv, r);

    glEnable(GL_DEPTH_TEST);
}

void frame_set_projection(frame_ptr f, GLfloat p[16])
{
    modelview_set_projection(f->mv, p);
}

struct worker_args
{
    char *path;
    engine_ptr e;
    frame_ptr f;
    renderer_ptr r;
};

void frame_rendernext_task(void *_wa)
{
    struct worker_args *wa = (struct worker_args *)_wa;
    engine_config_ptr ec = engine_get_config_ref(wa->e);

    // Check that the transition hasn't already completed
    if (wa->f->transition && wa->f->next_scene)
        wa->f->next_textureref = scene_draw(wa->f->next_scene, ec, wa->r);

    free(wa->path);
    free(wa);
}

/*
 * Load and render a new scene
 *
 * Call Context: Worker thread
 * TODO: Handle errors in worker threads
 */
void *frame_scene_load_worker(void *arg)
{
    clock_t start = clock();
    struct worker_args *wa = (struct worker_args *)arg;
    wa->f->next_scene = scene_create(wa->path, wa->f->width, wa->f->height, wa->e);
    wa->f->transition->loaded = true;
    printf("Loaded `%s' in %.1f ms\n", wa->path, (clock() - start)*1000.0f/CLOCKS_PER_SEC);

    // Update the textureref for the next frame after tick
    engine_queue_task(wa->e, frame_rendernext_task, wa);

    pthread_exit(NULL);
}

/*
 * Load a new scene with the requested path and transition type
 *
 * Call Context: Main thread
 */
void frame_load_scene(frame_ptr f, const char *path, const char *transition_type, engine_ptr e, renderer_ptr r)
{
    // TODO: Dirty hack to show a loadscreen until the scene has loaded
    f->next_textureref = texture_get_textureref(f->loadscreen, f->current_textureref.width, f->current_textureref.height);

    static pthread_t scene_load_worker;
    struct worker_args *wa = calloc(1, sizeof(struct worker_args));
    assert(wa);
    wa->path = strdup(path);
    wa->e = e;
    wa->f = f;
    wa->r = r;
    pthread_create(&scene_load_worker, NULL, frame_scene_load_worker, (void *)wa);

    assert(f->transition == NULL);
    f->transition = transition_instance_create(transition_type, f->quad, &f->current_textureref, &f->next_textureref, r);
}

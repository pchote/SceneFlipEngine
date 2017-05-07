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

/*
 * Engine is the entry point and controller for all platform independent
 * code (which should be everything except input and window creation).
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>

#include "engine.h"
#include "renderer.h"
#include "matrix.h"
#include "texture.h"
#include "modelview.h"
#include "frame.h"
#include "framebuffer.h"
#include "font.h"
#include "scene.h"
#include "walkmap.h"
#include "actor.h"

/*
 * Private implementation details
 */

struct engine_task
{
    void (*func)(void *);
    void *data;
    struct engine_task *next;
};

struct engine_synchronization_info
{
    volatile bool complete;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
};

struct texture_instance_list
{
    texture_ptr texture;
    uint32_t refcount;

    struct texture_instance_list *prev;
    struct texture_instance_list *next;
};

struct font_instance_list
{
    char *id;
    font_ptr font;
    uint32_t refcount;

    struct font_instance_list *next;
};

struct engine
{
    struct engine_config config;
    char *resource_path;
    renderer_ptr renderer;
    frame_ptr current_frame;

    GLuint window_width;
    GLuint window_height;

    input_flags input;
    GPpolar analog_input[2];

    // Task queue (as a linked list) for workers
    // to call a function from the main thread
    struct engine_task *tasks;
    struct engine_task **tasks_tail;
    pthread_mutex_t task_mutex;

    // Texture management
    struct texture_instance_list *textures;
    pthread_mutex_t texture_mutex;

    // Font management
    struct font_instance_list *fonts;
    struct font_instance_list **fonts_tail;
    pthread_mutex_t font_mutex;

    // For display feedback
    GLfloat tick_time;
    GLfloat task_time;
    GLuint fps;
    GLuint fps_count;
    time_t fps_time;
};

/*
 * Create an engine
 */
engine_ptr engine_create(const char *resource_path, GLuint window_width, GLuint window_height)
{
    engine_ptr e = calloc(1, sizeof(struct engine));
    if (!e)
        return NULL;

    e->tasks_tail = &e->tasks;
    e->resource_path = strdup(resource_path);
    chdir(e->resource_path);

    e->renderer = renderer_create();

    // TODO: Load from file
    e->config = (struct engine_config){
        .debug_render_layer_mesh = false,
        .debug_render_walkmesh = false,
        .debug_render_collisions = false,
        .debug_text_triangles = false,
        .resolution_height = 1536,
        .scene_aspect = 4.0f/3,
        .min_aspect = 1,
        .max_aspect = 1.5,
        .start_scene = strdup("space_test")
    };

    pthread_mutex_init(&e->texture_mutex, NULL);
    pthread_mutex_init(&e->task_mutex, NULL);

    e->fonts_tail = &e->fonts;
    pthread_mutex_init(&e->font_mutex, NULL);

    GLuint height = e->config.resolution_height;
    GLuint width = e->config.scene_aspect*height;
    e->current_frame = frame_create(e->config.start_scene, width, height, e, e->renderer);
    engine_set_viewport(e, window_width, window_height);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
    return e;
}

/*
 * Destroy engine state
 *
 * Call Context: Main thread
 */
void engine_destroy(engine_ptr e)
{
    assert(e);
    free(e->resource_path);
    renderer_destroy(e->renderer);
    frame_destroy(e->current_frame, e);

    // TODO: Clean up task queue without breaking
    // worker threads
    pthread_mutex_destroy(&e->task_mutex);

    for (struct texture_instance_list *tl = e->textures, *next; tl; tl = next)
    {
        assert(tl->refcount == 0);
        texture_destroy(tl->texture, e);
        next = tl->next;
        free(tl);
    }

    pthread_mutex_destroy(&e->texture_mutex);

    pthread_mutex_lock(&e->font_mutex);
    for (struct font_instance_list *fil = e->fonts, *next; fil; fil = next)
    {
        assert(fil->refcount == 0);
        font_destroy(fil->font, e);
        free(fil->id);
        next = fil->next;
        free(fil);
    }
    pthread_mutex_unlock(&e->font_mutex);

    free(e->config.start_scene);
    free(e);
}

#pragma mark Input
/*
 * Enable an input type
 */
void engine_enable_inputs(engine_ptr e, input_flags i)
{
    e->input |= i;
}

/*
 * Disable an input type
 */
void engine_disable_inputs(engine_ptr e, input_flags i)
{
    e->input &= ~i;
}

void engine_set_analog_input(engine_ptr e, analog_input_type type, GPpolar input)
{
    assert(type >= ANALOG_INPUT_DIRECTION && type <= ANALOG_INPUT_CAMERA);
    e->analog_input[type] = input;
}

void engine_update_overlay_display(engine_ptr e)
{
    frame_update_overlay_display(e->current_frame, &e->config);
}

#pragma mark Worker Task Management

/*
 * Queue a task to be run on the main thread
 *
 * Call Context: Worker thread
 */
void engine_queue_task(engine_ptr e, void (*func)(void *), void *data)
{
    struct engine_task *t = calloc(1, sizeof(struct engine_task));
    assert(t);
    t->func = func;
    t->data = data;

    // Append to list tail
    pthread_mutex_lock(&e->task_mutex);
    *e->tasks_tail = t;
    e->tasks_tail = &t->next;
    pthread_mutex_unlock(&e->task_mutex);
}

/*
 * Helper function for synchronizing the task queue
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void engine_task_synchronizer(void *_esi)
{
    struct engine_synchronization_info *esi = (struct engine_synchronization_info *)_esi;

    // Signal initialization complete
    pthread_mutex_lock(&esi->mutex);
    esi->complete = true;

    pthread_cond_signal(&esi->condition);
    pthread_mutex_unlock(&esi->mutex);
}

/*
 * Block until the current task queue has completed
 *
 * Call Context: Worker thread
 */
void engine_synchronize_tasks(engine_ptr e)
{
    struct engine_synchronization_info esi;
    esi.complete = false;
    pthread_mutex_init(&esi.mutex, NULL);
    pthread_cond_init(&esi.condition, NULL);

    // Block until complete
    pthread_mutex_lock(&esi.mutex);
    engine_queue_task(e, engine_task_synchronizer, &esi);

    while(!esi.complete)
        pthread_cond_wait(&esi.condition, &esi.mutex);
    pthread_mutex_unlock(&esi.mutex);

    pthread_cond_destroy(&esi.condition);
    pthread_mutex_destroy(&esi.mutex);
}

/*
 * Process tasks that were queued by worker threads
 * Run as many tasks as possible within time_threshold seconds
 *
 * Call Context: Main thread
 */
void engine_process_tasks(engine_ptr e, double time_threshold)
{
    // No tasks
    if (!e->tasks)
        return;

    pthread_mutex_lock(&e->task_mutex);
    clock_t start = clock();
    double elapsed;
    do
    {
        // Pop the first task from the queue
        struct engine_task *t = e->tasks;
        e->tasks = t->next;
        if (!e->tasks)
            e->tasks_tail = &e->tasks;

        t->func(t->data);
        free(t);
        elapsed = (clock() - start)*1.0f/CLOCKS_PER_SEC;
    }
    while (e->tasks && elapsed < time_threshold);
    pthread_mutex_unlock(&e->task_mutex);
}

#pragma mark Worker Texture Management
/*
 * Load a texture, or increase the refcount if it is already loaded
 *
 * Call Context: Worker thread
 */
texture_instance_ptr engine_retain_texture(engine_ptr e, const char *path)
{
    pthread_mutex_lock(&e->texture_mutex);

    // Search for existing texture
    struct texture_instance_list **end = &e->textures;
    for (; *end; end = &(*end)->next)
        if (texture_has_path((*end)->texture, path))
        {
            (*end)->refcount++;
            pthread_mutex_unlock(&e->texture_mutex);
            return (*end)->texture;
        }

    // Create new texture
    struct texture_instance_list *tl = calloc(1, sizeof(struct texture_instance_list));
    assert(tl);
    tl->refcount = 1;
    tl->texture = texture_create(path, e);
    assert(tl->texture);

    tl->prev = *end;
    *end = tl;

    pthread_mutex_unlock(&e->texture_mutex);
    return tl->texture;
}

/*
 * Decrease the refcount on a texture. Free it if it hits zero
 *
 * Call Context: Worker thread
 */
void engine_release_texture(engine_ptr e, texture_instance_ptr t)
{
    pthread_mutex_lock(&e->texture_mutex);

    // Find instance in the textures array
    struct texture_instance_list *tl = NULL;
    for (tl = e->textures; tl; tl = tl->next)
        if (tl->texture == t)
        {
            // Decrement refcount. If nonzero, we're done
            if (--tl->refcount)
                break;

            // Free texture
            engine_queue_task(e, (void (*)(void *))texture_destroy_internal, tl->texture);

            if (tl->prev)
                tl->prev->next = tl->next;

            if (tl->next)
                tl->next->prev = tl->prev;

            free(tl);
            break;
        }

    if (tl == NULL)
    {
        printf("Attempting to release a non-retained texture\n");
        assert(FATAL_ERROR);
    }

    pthread_mutex_unlock(&e->texture_mutex);
}

void engine_load_font(engine_ptr e, const char *id, const char *file, GLuint size)
{
    GLsizei font_tex_size = 512;
    GLuint width = e->config.scene_aspect*e->config.resolution_height;

    struct font_instance_list *fi = calloc(1, sizeof(struct font_instance_list));
    assert(fi);

    fi->id = strdup(id);
    assert(fi->id);

    fi->font = font_create("Inconsolata.otf", size, font_tex_size, font_tex_size*4.0f/width, e);

    *e->fonts_tail = fi;
    e->fonts_tail = &fi->next;
}

font_instance_ptr engine_retain_font(engine_ptr e, const char *id)
{
    pthread_mutex_lock(&e->font_mutex);
    for (struct font_instance_list *fi = e->fonts; fi; fi = fi->next)
        if (strcmp(fi->id, id) == 0)
        {
            fi->refcount++;
            pthread_mutex_unlock(&e->font_mutex);
            return fi->font;
        }

    pthread_mutex_unlock(&e->font_mutex);
    printf("Attempting to reference unknown font '%s'\n", id);
    assert(FATAL_ERROR);
}

void engine_release_font(engine_ptr e, font_instance_ptr fi)
{
    pthread_mutex_lock(&e->font_mutex);
    for (struct font_instance_list *fil = e->fonts; fil; fil = fil->next)
        if (fi == fil->font)
        {
            fil->refcount--;
            return;
        }
    pthread_mutex_unlock(&e->font_mutex);
    printf("Attempting to release unknown font\n");
    assert(FATAL_ERROR);
}

# pragma mark Misc
/*
 * Tick the scene state
 * dt is the elapsed time in seconds
 *
 * Call Context: Main thread
 */
void engine_tick(engine_ptr e, double dt)
{
    time_t t = time(NULL);
    if (t != e->fps_time)
    {
        e->fps = e->fps_count;
        e->fps_count = 0;
        e->fps_time = t;
    }
    else
        e->fps_count++;

    // Place a limit of 10ms each tick for tasks
    // Leaves ~20ms for tick/render if we want to
    // keep to 30fps
    clock_t start = clock();
    engine_process_tasks(e, 0.01);
    clock_t after_tasks = clock();

    if (dt > 0.5)
    {
        printf("Long tick (%f). Ignoring\n", dt);
        return;
    }

    frame_tick(e->current_frame, dt, e, e->renderer);
    clock_t after_tick = clock();

    e->task_time = (after_tasks - start)*1.0f/CLOCKS_PER_SEC;
    e->tick_time = (after_tick - after_tasks)*1.0f/CLOCKS_PER_SEC;
}

/*
 * Start a transition to a new scene
 */
void engine_transition_to_scene(engine_ptr e, const char *path, const char *transition_type)
{
    frame_load_scene(e->current_frame, path, transition_type, e, e->renderer);
}

/*
 * Draw a frame to the current GL context
 */
void engine_draw(engine_ptr e)
{
    glViewport(0, 0, e->window_width, e->window_height); checkGLError();
    glClearColor(0, 0, 0, 0); checkGLError();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); checkGLError();
    frame_draw(e->current_frame, e->fps, e->tick_time, e->task_time, e, e->renderer);
    glFlush();
}

/*
 * Set (or update) the viewport size
 * The coordinate system is set so that the smaller axis
 * maps from 0->1, and the larger axis has 0->1 centered
 */
void engine_set_viewport(engine_ptr e, GLuint width, GLuint height)
{
    e->window_width = width;
    e->window_height = height;

    // TODO: Account for min and max aspect ratios

    // Ensure x and y scales remain constant and that
    // the unit square remains visible at all times
    GLfloat w = 1;
    GLfloat h = 1;
    if (width > height)
        w = width*1.0f/height;
    else
        h = height*1.0f/width;

    GLfloat projection[16];
    mtxLoadOrthographic(projection, -w, w, -h, h, 0, 1);
    frame_set_projection(e->current_frame, projection);
}

#pragma mark Getters for external objects
input_flags engine_discrete_inputs(engine_ptr e)
{
    return e->input;
}

GPpolar engine_analog_inputs(engine_ptr e, analog_input_type type)
{
    assert(type >= ANALOG_INPUT_DIRECTION && type <= ANALOG_INPUT_CAMERA);
    return e->analog_input[type];
}

engine_config_ptr engine_get_config_ref(engine_ptr e)
{
    return &e->config;
}

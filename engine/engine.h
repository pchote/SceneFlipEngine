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

#ifndef GamePrototype_engine_h
#define GamePrototype_engine_h

#include "typedefs.h"

struct engine_config
{
    // lua file for first scene
    char *start_scene;

    // Aspect ratio used for scenes
    GLfloat scene_aspect;

    // Height component for rendering scenes
    // (width is calculated from scene_aspect)
    GLfloat resolution_height;

    // Minimum and maximum aspect ratio used for display
    GLfloat min_aspect;
    GLfloat max_aspect;

    // Flags for enabling debug rendering modes
    bool debug_render_layer_mesh;
    bool debug_render_walkmesh;
    bool debug_render_collisions;
    bool debug_text_triangles;
};

engine_ptr engine_create(const char *resource_path, GLuint window_width, GLuint window_height);
void engine_destroy(engine_ptr e);
void engine_draw(engine_ptr e);
void engine_set_viewport(engine_ptr e, GLuint width, GLuint height);

void engine_enable_inputs(engine_ptr e, input_flags i);
void engine_disable_inputs(engine_ptr e, input_flags i);
void engine_set_analog_input(engine_ptr e, analog_input_type type, GPpolar input);
void engine_update_overlay_display(engine_ptr e);

void engine_tick(engine_ptr e, double dt);

input_flags engine_discrete_inputs(engine_ptr e);
GPpolar engine_analog_inputs(engine_ptr e, analog_input_type type);
engine_config_ptr engine_get_config_ref(engine_ptr e);
void engine_transition_to_scene(engine_ptr e, const char *path, const char *transition_type);
void engine_queue_task(engine_ptr e, void (*func)(void *), void *data);
void engine_synchronize_tasks(engine_ptr e);

texture_instance_ptr engine_retain_texture(engine_ptr e, const char *path);
void engine_release_texture(engine_ptr e, texture_instance_ptr t);

void engine_load_font(engine_ptr e, const char *id, const char *file, GLuint size);
font_instance_ptr engine_retain_font(engine_ptr e, const char *id);
void engine_release_font(engine_ptr e, font_instance_ptr fi);

#endif

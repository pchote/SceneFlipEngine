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

#ifndef GamePrototype_scene_h
#define GamePrototype_scene_h

#include "typedefs.h"

struct camera_state
{
    GLfloat fov;
    GLfloat pos[3];
    GLfloat pitch;
    GLfloat yaw;
    GLfloat z_near;
    GLfloat z_far;

    GLfloat aspect;
    GLfloat perspective[16];
    GPpolar debug_offset;
};

scene_ptr scene_create(const char *scene_path, GLuint resolution, GLuint aspect, engine_ptr e);
void scene_destroy(scene_ptr s, engine_ptr e);
void scene_tick(scene_ptr s, engine_ptr e, double dt);

struct camera_state scene_camera(scene_ptr s);
void scene_update_camera(scene_ptr s, GPpolar offset);

textureref scene_draw(scene_ptr s, engine_config_ptr ec, renderer_ptr r);

actor_ptr scene_load_actor(scene_ptr s, const char *model, GLfloat collision_radius, engine_ptr e);
void scene_add_actor(scene_ptr s, actor_ptr a);
void scene_remove_actor(scene_ptr s, actor_ptr a);

layer_ptr scene_load_layer(scene_ptr s, const char *image, GLfloat *screen_region, GLfloat depth,
                           GLfloat *frame_regions, GLsizei frame_count, GLfloat *normal, engine_ptr e);

void scene_add_trigger_region(scene_ptr s, GLfloat pos[3], GLfloat *vertices, GLsizei vertex_count, int callback, engine_ptr e);
void scene_add_timeout(scene_ptr s, int callback, GLfloat ms);

#endif

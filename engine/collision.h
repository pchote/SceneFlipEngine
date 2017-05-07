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

#ifndef GamePrototype_collision_h
#define GamePrototype_collision_h

#include "typedefs.h"

typedef struct collision_object *collision_object_t;
typedef struct collision_world *collision_world_t;
typedef struct collision_iterator *collision_iterator_t;

collision_object_t collision_object_create_circle(collision_world_t w, GLfloat pos[2], GLfloat radius, void *userdata);
collision_object_t collision_object_create_polygon(collision_world_t world, GLfloat *vertices, GLsizei vertex_count, uint16_t group, uint16_t mask_flags, void *userdata);
collision_object_t collision_object_create_triangle(collision_world_t w, GLfloat a[2], GLfloat b[2], GLfloat c[2], uint16_t group, uint16_t mask_flags, void *userdata);
collision_object_t collision_object_create_chain(collision_world_t w, GLfloat *vertices_3d, uint32_t vertex_count, uint16_t group, uint16_t mask_flags, void *userdata);
void collision_object_free(collision_object_t o, collision_world_t w);
void collision_object_copy_collisiondata(collision_object_t to, collision_object_t from);
void collision_object_position(collision_object_t o, GLfloat pos[2]);
void collision_object_set_position(collision_object_t o, GLfloat p[2]);
void collision_object_velocity(collision_object_t o, GLfloat v[2]);
void collision_object_set_velocity(collision_object_t o, GLfloat v[2]);
uint16_t collision_object_collision_mask(collision_object_t o);

collision_world_t collision_world_create();
void collision_world_free(collision_world_t w);
size_t collision_world_count(collision_world_t w);
void collision_world_tick(collision_world_t w, double dt);
void *collision_world_hittest(collision_world_t w, GLfloat pos[2], void *(*callback)(void *, void *, void *), void *callbackdata);

collision_iterator_t collision_iterator_create(collision_world_t w);
void collision_iterator_free(collision_iterator_t it);
void *collision_iterator_userdata(collision_iterator_t it);
void collision_iterator_advance(collision_iterator_t it);
bool collision_iterator_finished(collision_iterator_t it);

bool collision_object_hittest(collision_object_t co, GLfloat p[2], uint16_t collision_flags);

#endif

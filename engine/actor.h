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

#ifndef GamePrototype_actor_h
#define GamePrototype_actor_h

#include "typedefs.h"

actor_ptr actor_create(const char *model, GLfloat collision_radius, walkmap_ptr w, engine_ptr e);
void actor_destroy(actor_ptr a, walkmap_ptr w, engine_ptr e);
void actor_draw(actor_ptr a, modelview_ptr mv, renderer_ptr r);

void actor_velocity(actor_ptr a, GLfloat v[2], walkmap_ptr w);
void actor_set_velocity(actor_ptr a, GLfloat v[2], walkmap_ptr w);
void actor_position(actor_ptr a, GLfloat p[3], walkmap_ptr w);
void actor_set_position(actor_ptr a, GLfloat p[3], walkmap_ptr w);

void actor_add_to_walkmap(actor_ptr a, GLfloat pos[3], GLfloat facing, walkmap_ptr w);
void actor_remove_from_walkmap(actor_ptr a, walkmap_ptr w);

#endif

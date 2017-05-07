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

#ifndef GamePrototype_walkmap_h
#define GamePrototype_walkmap_h

#include "typedefs.h"

walkmap_ptr walkmap_create(const char *mesh_path, engine_ptr e);
void walkmap_destroy(walkmap_ptr w, engine_ptr e);
void walkmap_debug_draw_walkmesh(walkmap_ptr w, modelview_ptr mv, renderer_ptr r);
void walkmap_debug_draw_collisions(walkmap_ptr w, modelview_ptr mv, renderer_ptr r);
void walkmap_tick(walkmap_ptr w, double dt);
void walkmap_check_triggers(walkmap_ptr w, scene_ptr s, void (*trigger_callback)(scene_ptr, actor_ptr, int));

walkmap_actordata_ptr walkmap_register_actor(walkmap_ptr w, actor_ptr a, GLfloat pos[3], GLfloat radius);
void walkmap_unregister_actor(walkmap_ptr w, walkmap_actordata_ptr ad);
void walkmap_set_movement_callback(walkmap_ptr w, walkmap_actordata_ptr ad, void (*movement_callback)(actor_ptr, GLfloat[3], GLfloat[3]));

void walkmap_actor_position(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat p[3]);
void walkmap_set_actor_position(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat p[3]);
void walkmap_actor_velocity(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat v[2]);
void walkmap_set_actor_velocity(walkmap_ptr w, walkmap_actordata_ptr ad, GLfloat v[2]);

void walkmap_register_trigger_region(walkmap_ptr w, GLfloat pos[3], GLfloat *vertices, GLsizei vertex_count, int callback, engine_ptr e);

#endif

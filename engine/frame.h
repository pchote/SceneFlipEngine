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

#ifndef GPEngine_frame_h
#define GPEngine_frame_h

#include "typedefs.h"

frame_ptr frame_create(const char *start_scene, GLuint width, GLuint height, engine_ptr e, renderer_ptr r);
void frame_destroy(frame_ptr f, engine_ptr e);
void frame_tick(frame_ptr f, double dt, engine_ptr e, renderer_ptr r);
void frame_draw(frame_ptr f, GLuint fps, GLfloat tick_time, GLfloat task_time, engine_ptr e, renderer_ptr r);
void frame_set_projection(frame_ptr f, GLfloat p[16]);
void frame_load_scene(frame_ptr f, const char *path, const char *transition_type, engine_ptr e, renderer_ptr r);
void frame_update_overlay_display(frame_ptr f, engine_config_ptr ec);

#endif

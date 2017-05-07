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

#ifndef GPEngine_texture_h
#define GPEngine_texture_h

#include "typedefs.h"

texture_ptr texture_create(const char *path, engine_ptr e);
void texture_destroy(texture_ptr t, engine_ptr e);
void texture_destroy_internal(texture_ptr t);

void texture_bind(texture_instance_ptr t, GLenum unit);
bool texture_has_path(texture_ptr t, const char *path);
textureref texture_get_textureref(texture_ptr t, GLfloat width, GLfloat height);

#endif

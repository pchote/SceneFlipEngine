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

#ifndef GPEngine_framebuffer_h
#define GPEngine_framebuffer_h

#include "typedefs.h"

GLuint framebuffer_size(GLuint width, GLuint height);

framebuffer_ptr framebuffer_create(GLuint width, GLuint height, engine_ptr e);
void framebuffer_destroy(framebuffer_ptr fb, engine_ptr e);

void framebuffer_bind(framebuffer_ptr fb);
void framebuffer_unbind(framebuffer_ptr fb);

textureref framebuffer_get_textureref(framebuffer_ptr fb);

#endif

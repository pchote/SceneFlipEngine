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

#ifndef GPEngine_vertexarray_h
#define GPEngine_vertexarray_h

#include "typedefs.h"

vertexarray_ptr vertexarray_create_quad(GLfloat width, GLfloat height, engine_ptr e);
vertexarray_ptr vertexarray_create(GLfloat *vertices, GLfloat *texcoords, GLsizei vertex_count,
                                   GLsizei texcoord_size, GLenum type, engine_ptr e);
void vertexarray_destroy(vertexarray_ptr va, engine_ptr e);

void vertexarray_update(vertexarray_ptr va, GLfloat *vertices, GLfloat *texcoords, GLsizei count, GLenum type);
void vertexarray_draw(vertexarray_ptr va);

#endif

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

#ifndef GPEngine_modelview_h
#define GPEngine_modelview_h

#include "typedefs.h"

modelview_ptr modelview_create();
void modelview_destroy(modelview_ptr mv);
void modelview_set_projection(modelview_ptr mv, GLfloat p[16]);
void modelview_set_camera(modelview_ptr mv, GLfloat c[16]);
GLfloat *modelview_push(modelview_ptr mv);
void modelview_pop(modelview_ptr mv);
void modelview_calculate_mvp(modelview_ptr mv, GLfloat mvp[16]);

#endif

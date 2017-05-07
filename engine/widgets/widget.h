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

#ifndef GPEngine_widget_h
#define GPEngine_widget_h

#include "typedefs.h"

enum widget_type {WIDGET_CONTAINER, WIDGET_STRING};

widget_ptr widget_create_root();
void widget_add(widget_ptr parent, const char *id, GLfloat pos[2], enum widget_type type, void *data);
void widget_destroy(widget_ptr w, engine_ptr e);
void widget_draw(widget_ptr w, modelview_ptr mv, renderer_ptr r);
void widget_debug_draw(widget_ptr w, modelview_ptr mv, renderer_ptr r);


#endif

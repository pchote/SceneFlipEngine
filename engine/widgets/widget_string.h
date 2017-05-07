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

#ifndef GPEngine_widget_string_h
#define GPEngine_widget_string_h

#include "typedefs.h"

widget_string_ptr widget_string_create(const char *font_id, engine_ptr e);
void widget_string_destroy(widget_string_ptr ws, engine_ptr e);
void widget_string_draw(widget_string_ptr ws, modelview_ptr mv, renderer_ptr r);
void widget_string_debug_draw(widget_string_ptr ws, modelview_ptr mv, renderer_ptr r);
void widget_string_set_text(widget_string_ptr ws, char *text, GLenum lifetime);

#endif

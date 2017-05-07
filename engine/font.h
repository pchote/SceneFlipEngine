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

#ifndef GamePrototype_font_h
#define GamePrototype_font_h

#include "typedefs.h"

font_ptr font_create(const char *path, GLuint font_size, GLuint texture_size, GLfloat scale, engine_ptr e);
void font_destroy(font_ptr f, engine_ptr e);
void font_bind_texture(font_instance_ptr f);
GLsizei font_string_glyph_count(font_instance_ptr f, char *string);
void font_render_string(font_instance_ptr f, const char *str, GLsizei len, GLfloat *buffer);

#endif

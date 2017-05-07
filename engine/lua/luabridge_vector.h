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

#ifndef GPEngine_luabridge_vector_h
#define GPEngine_luabridge_vector_h

#include <lua.h>
#include "typedefs.h"

void luabridge_register_vectors(lua_State *L);
void luabridge_push_vector(lua_State *L, GLfloat *data, uint8_t n);
GLfloat *luabridge_check_vector(lua_State *L, int ud, uint8_t n);

#endif

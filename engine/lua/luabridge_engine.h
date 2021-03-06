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

#ifndef GPEngine_luabridge_engine_h
#define GPEngine_luabridge_engine_h

#include <lua.h>
#include "typedefs.h"

#define LUABRIDGE_ENGINE_TYPENAME "engine"

typedef struct
{
    engine_ptr engine;
} luabridge_engineref;

void luabridge_push_engineref(lua_State *L, engine_ptr e);
luabridge_engineref *luabridge_check_engineref(lua_State *L, int pos);
void luabridge_register_engineref(lua_State *L);

#endif

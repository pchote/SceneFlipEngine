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

#ifndef GPEngine_luabridge_layer_h
#define GPEngine_luabridge_layer_h

#include <lua.h>
#include "typedefs.h"

#define LUABRIDGE_LAYER_TYPENAME "layer"

typedef struct
{
    layer_ptr layer;
} luabridge_layerref;

void luabridge_push_layerref(lua_State *L, layer_ptr l);
luabridge_layerref *luabridge_check_layerref(lua_State *L, int pos);
void luabridge_register_layerref(lua_State *L);

#endif

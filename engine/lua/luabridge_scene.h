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

#ifndef GPEngine_luabridge_scene_h
#define GPEngine_luabridge_scene_h

#include <lua.h>
#include "typedefs.h"

#define LUABRIDGE_SCENE_TYPENAME "scene"

typedef struct
{
    scene_ptr scene;
} luabridge_sceneref;

void luabridge_push_sceneref(lua_State *L, scene_ptr s);
luabridge_sceneref *luabridge_check_sceneref(lua_State *L, int pos);
void luabridge_register_sceneref(lua_State *L);
void luabridge_scene_run_trigger(lua_State *L, actor_ptr a, int callback);
void luabridge_scene_run_timeout(lua_State *L, int callback, GLfloat overflow);

#endif

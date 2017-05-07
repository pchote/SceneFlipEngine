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

#ifndef GPEngine_luabridge_h
#define GPEngine_luabridge_h

#include <lua.h>
#include "typedefs.h"
#include "scene.h"

size_t luabridge_count_table_elements(lua_State *L, int idx);

engine_ptr luabridge_registry_get_engine(lua_State *L);
scene_ptr luabridge_registry_get_scene(lua_State *L);
walkmap_ptr luabridge_registry_get_walkmap(lua_State *L);
void luabridge_assert_setup(lua_State *L);

void luabridge_set_globals(lua_State *L, scene_ptr s, walkmap_ptr w, engine_ptr e, bool in_setup);
void luabridge_clear_globals(lua_State *L);

void luabridge_run_setup(lua_State *L, scene_ptr s);
void luabridge_run_tick(lua_State *L, scene_ptr s, engine_ptr e, double dt);

lua_State *luabridge_load(const char *path);
void luabridge_parse_scene_camera(lua_State *L, struct camera_state *camera);
void luabridge_destroy_ref(lua_State *L, int ref);

#endif

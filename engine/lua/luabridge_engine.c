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

#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>

#include "luabridge.h"
#include "luabridge_engine.h"
#include "luabridge_vector.h"
#include "engine.h"

/*
 * Push an engine reference onto the lua stack
 */
void luabridge_push_engineref(lua_State *L, engine_ptr e)
{
    luabridge_engineref *er = lua_newuserdata(L, sizeof(luabridge_engineref));
    luaL_setmetatable(L, LUABRIDGE_ENGINE_TYPENAME);
    er->engine = e;
}

/*
 *  (void) engine:loadScene(string scene, string transition)
 */
static int load_scene(lua_State *L)
{
    luabridge_engineref *er = luaL_checkudata(L, 1, LUABRIDGE_ENGINE_TYPENAME);
    const char *scene = luaL_checkstring(L, 2);
    const char *transition = luaL_checkstring(L, 3);

    // TODO: check that scene and transition both exist
    // and generate an error instead of crashing
    engine_transition_to_scene(er->engine, scene, transition);

    return 0;
}

/*
 *  (table) engine:getInput(void)
 */
static int get_input(lua_State *L)
{
    luabridge_engineref *er = luaL_checkudata(L, 1, LUABRIDGE_ENGINE_TYPENAME);
    input_flags d = engine_discrete_inputs(er->engine);

    lua_newtable(L);
    lua_pushboolean(L, d & INPUT_RESET_CAMERA);
    lua_setfield(L, -2, "reset_camera");

    GPpolar dir = engine_analog_inputs(er->engine, ANALOG_INPUT_DIRECTION);
    luabridge_push_vector(L, (GLfloat[]){dir.radius, dir.angle}, 2);
    lua_setfield(L, -2, "analog_direction");

    GPpolar cam = engine_analog_inputs(er->engine, ANALOG_INPUT_CAMERA);
    luabridge_push_vector(L, (GLfloat[]){cam.radius, cam.angle}, 2);
    lua_setfield(L, -2, "analog_camera");
    return 1;
}

/*
 *  (string) engine:__tostring(void)
 */
static int description(lua_State *L)
{
    luabridge_engineref *er = luaL_checkudata(L, 1, LUABRIDGE_ENGINE_TYPENAME);
    lua_pushfstring(L, LUABRIDGE_ENGINE_TYPENAME"(%p)", er->engine);
    return 1;
}

/*
 * Register the 'engineref' type with lua
 */
void luabridge_register_engineref(lua_State *L)
{
    const luaL_Reg methods[] = {
        {"getInput", get_input},
        {"loadScene", load_scene},
        {"__tostring", description},
        {NULL, NULL}
    };

    // Register methods with the engineref metatable
    luaL_newmetatable(L, LUABRIDGE_ENGINE_TYPENAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);
}

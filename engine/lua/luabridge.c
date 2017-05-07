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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <lualib.h>
#include <lauxlib.h>
#include <setjmp.h>

#include "luabridge.h"
#include "luabridge_engine.h"
#include "luabridge_scene.h"
#include "luabridge_actor.h"
#include "luabridge_layer.h"
#include "luabridge_vector.h"

// Use pointers to unique strings as registry keys
static char *registry_engine_key = "gp_engine_key";
static char *registry_scene_key = "gp_scene_key";
static char *registry_walkmap_key = "gp_walkmap_key";
static char *registry_setup_key = "gp_setup_key";

engine_ptr luabridge_registry_get_engine(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, registry_engine_key);
    if (!lua_islightuserdata(L, -1))
        luaL_error(L, "Failed to fetch engine from registry");
    return lua_touserdata(L, -1);
}

scene_ptr luabridge_registry_get_scene(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, registry_scene_key);
    if (!lua_islightuserdata(L, -1))
        luaL_error(L, "Failed to fetch scene from registry");
    return lua_touserdata(L, -1);
}

walkmap_ptr luabridge_registry_get_walkmap(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, registry_walkmap_key);
    if (!lua_islightuserdata(L, -1))
        luaL_error(L, "Failed to fetch walkmap from registry");
    return lua_touserdata(L, -1);
}

void luabridge_assert_setup(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, registry_setup_key);
    if (!lua_toboolean(L, -1))
        luaL_error(L, "This function can only be called during setup");
    lua_pop(L, 1);
}

static void luabridge_registry_set_scene(lua_State *L, scene_ptr s)
{
    luabridge_push_sceneref(L, s);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_scene_key);
}

static void luabridge_registry_clear_scene(lua_State *L)
{
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_scene_key);
}

#pragma mark Helper functions

/*
* Count the number of entries in a table at idx
* TODO: lua_rawlen should do this, but always returns 0
*/
size_t luabridge_count_table_elements(lua_State *L, int idx)
{
    int index = lua_absindex(L, idx);
    size_t i = 0;
    lua_pushnil(L);
    while (lua_next(L, index))
    {
        i++;
        lua_pop(L, 1);
    }
    return i;
}

#pragma mark C Interface

void luabridge_set_globals(lua_State *L, scene_ptr s, walkmap_ptr w, engine_ptr e, bool in_setup)
{
    lua_pushlightuserdata(L, s);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_scene_key);
    lua_pushlightuserdata(L, w);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_walkmap_key);
    lua_pushlightuserdata(L, e);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_engine_key);
    lua_pushboolean(L, in_setup);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_setup_key);

    luabridge_push_engineref(L, e);
    lua_setglobal(L, "engine");

    luabridge_push_sceneref(L, s);
    lua_setglobal(L, "scene");
}

void luabridge_clear_globals(lua_State *L)
{
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_scene_key);
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_walkmap_key);
    lua_pushnil(L);
    lua_setfield(L, LUA_REGISTRYINDEX, registry_engine_key);

    lua_pushnil(L);
    lua_setglobal(L, "engine");

    lua_pushnil(L);
    lua_setglobal(L, "scene");
}

/*
 * Run the lua 'setup(sceneref)' function
 */
void luabridge_run_setup(lua_State *L, scene_ptr s)
{
    jmp_buf error;
    if (!setjmp(error))
    {
        lua_getglobal(L, "setup");
        if (lua_pcall(L, 0, 0, 0))
        {
            fprintf(stderr, "Scene setup failed.\n%s\n", lua_tostring(L, -1));
            assert(FATAL_ERROR);
        }
    }
    else
    {
        fprintf(stderr, "Scene setup failed: %s\n", lua_tostring(L, -1));
        assert(FATAL_ERROR);
    }
}

/*
 * Call the lua 'tick(sceneref)' function
 */
void luabridge_run_tick(lua_State *L, scene_ptr s, engine_ptr e, double dt)
{
    lua_getglobal(L, "tick");
    lua_pushnumber(L, dt);

    if (lua_pcall(L, 1, 0, 0))
    {
        fprintf(stderr, "Scene tick failed.\n%s\n", lua_tostring(L, -1));
        assert(FATAL_ERROR);
    }
}

/*
 * Jumped to by lua on a parse error
 * Jumps to the state set in error_jump
 */
jmp_buf error_jmp;
static int load_parse_error(lua_State *L)
{
    longjmp(error_jmp, 1);
    return 0;
}

/*
 * Load a lua file into memory
 * Dies if there is a syntax error
 */
lua_State *luabridge_load(const char *path)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    luabridge_register_sceneref(L);
    luabridge_register_engineref(L);
    luabridge_register_actorref(L);
    luabridge_register_layerref(L);
    luabridge_register_vectors(L);

    if (luaL_dofile(L, path))
    {
        fprintf(stderr, "Error parsing %s: %s\n", path, lua_tostring(L, -1));
        assert(FATAL_ERROR);
    }

    return L;
}

/*
 * Parse scene metadata into the given scene file
 */
void luabridge_parse_scene_camera(lua_State *L, struct camera_state *camera)
{
    // Set a jump buffer to return to on parse error
    if (!setjmp(error_jmp))
    {
        lua_atpanic(L, load_parse_error);

        lua_getglobal(L, "camera");
        if (!lua_istable(L, -1))
            luaL_error(L, "`camera' is not a valid table");

        camera->debug_offset = (GPpolar){0, 0};
        lua_getfield(L, -1, "fov");
        camera->fov = luaL_checknumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "pos");
        GLfloat *pos = luabridge_check_vector(L, -1, 3);
        memcpy(camera->pos, pos, 3*sizeof(GLfloat));
        lua_pop(L, 1);

        lua_getfield(L, -1, "pitch");
        camera->pitch = luaL_checknumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "yaw");
        camera->yaw = luaL_checknumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "z_near");
        camera->z_near = luaL_checknumber(L, -1);
        lua_pop(L, 1);

        lua_getfield(L, -1, "z_far");
        camera->z_far = luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }
    else
    {
        fprintf(stderr, "Error parsing scene metadata: %s\n", lua_tostring(L, -1));
        assert(FATAL_ERROR);
    }
}

/*
 * Remove a reference in the lua registry created with luaL_ref
 */
void luabridge_destroy_ref(lua_State *L, int ref)
{
    luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

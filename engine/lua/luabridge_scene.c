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
#include <stdlib.h>
#include <string.h>

#include "luabridge.h"
#include "luabridge_engine.h"
#include "luabridge_scene.h"
#include "luabridge_actor.h"
#include "luabridge_layer.h"
#include "luabridge_vector.h"
#include "scene.h"

/*
 * Push a scene reference onto the lua stack
 */
void luabridge_push_sceneref(lua_State *L, scene_ptr s)
{
    luabridge_sceneref *sr = lua_newuserdata(L, sizeof(luabridge_sceneref));
    luaL_setmetatable(L, LUABRIDGE_SCENE_TYPENAME);
    sr->scene = s;
}

/*
 *  (vec2) scene:getCameraOffset()
 */
static int get_camera_offset(lua_State *L)
{
    luabridge_sceneref *sr = luaL_checkudata(L, 1, LUABRIDGE_SCENE_TYPENAME);

    GPpolar offset = scene_camera(sr->scene).debug_offset;
    luabridge_push_vector(L, (GLfloat[]){offset.radius, offset.angle}, 2);
    return 1;
}

/*
 *  (void) scene:setCameraOffset(vec2 offset)
 */
static int set_camera_offset(lua_State *L)
{
    luabridge_sceneref *sr = luaL_checkudata(L, 1, LUABRIDGE_SCENE_TYPENAME);
    GLfloat *v = luabridge_check_vector(L, 2, 2);
    scene_update_camera(sr->scene, (GPpolar){v[0], v[1]});
    return 0;
}

/*
 *  (actorref) scene:loadActor(string model [, number radius])
 */
static int load_actor(lua_State *L)
{
    luabridge_assert_setup(L);

    luabridge_sceneref *sr = luaL_checkudata(L, 1, LUABRIDGE_SCENE_TYPENAME);
    int args = lua_gettop(L);
    const char *model = luaL_checkstring(L, 2);
    GLfloat collision_radius = (args == 3) ? luaL_checknumber(L, 3) : 0.5;

    engine_ptr e = luabridge_registry_get_engine(L);
    luabridge_push_actorref(L, scene_load_actor(sr->scene, model, collision_radius, e));
    return 1;
}

/*
 *  (void) scene:loadLayer(string image, vec4 screen_region, number screen_depth,
 *                         table<vec4> frame_regions [, vec3 normal])
 */
static int load_layer(lua_State *L)
{
    luabridge_assert_setup(L);

    luabridge_sceneref *sr = luaL_checkudata(L, 1, LUABRIDGE_SCENE_TYPENAME);
    int arg_count = lua_gettop(L);
    const char *image = luaL_checkstring(L, 2);

    // First arg is the layer object
    if (arg_count < 5)
        luaL_error(L, "loadLayer requires 4 or 5 args");

    GLfloat *screen_region = luabridge_check_vector(L, 3, 4);
    GLfloat depth = luaL_checknumber(L, 4);

    luaL_checktype(L, 5, LUA_TTABLE);
    GLsizei frame_count = (GLsizei)luabridge_count_table_elements(L, 5);
    GLfloat *frame_regions = calloc(4*frame_count, sizeof(GLfloat));
    if (!frame_regions)
        luaL_error(L, "Allocation error");

    lua_pushnil(L);
    for (size_t i = 0; i < frame_count; i++)
    {
        lua_next(L, 5);
        GLfloat *frame_region = luabridge_check_vector(L, -1, 4);
        if (!frame_region)
            luaL_error(L, "Invalid vec4 in region list (argument 4)");
        memcpy(&frame_regions[4*i], frame_region, 4*sizeof(GLfloat));
        lua_pop(L, 1);
    }

    // May be NULL if not given in lua
    // scene_load_layer will set the normal to point at the camera in this case
    GLfloat *normal = arg_count > 5 ? luabridge_check_vector(L, 6, 3) : (GLfloat[]){0, 1, 0};

    engine_ptr e = luabridge_registry_get_engine(L);
    layer_ptr l = scene_load_layer(sr->scene, image, screen_region, depth,
                                   frame_regions, frame_count, normal, e);
    luabridge_push_layerref(L, l);
    free(frame_regions);

    return 1;
}

/*
 *  (void) scene:addTrigger(vec3 position, table<vec2> points, function callback)
 */
static int add_trigger(lua_State *L)
{
    luabridge_assert_setup(L);

    luabridge_sceneref *sr = luaL_checkudata(L, 1, LUABRIDGE_SCENE_TYPENAME);
    GLfloat *pos = luabridge_check_vector(L, 2, 3);

    luaL_checktype(L, 3, LUA_TTABLE);
    GLsizei vertex_count = (GLsizei)luabridge_count_table_elements(L, 3);
    GLfloat *vertices = calloc(2*vertex_count, sizeof(GLfloat));
    if (!vertices)
        luaL_error(L, "Allocation error");

    lua_pushnil(L);
    for (size_t i = 0; i < vertex_count; i++)
    {
        lua_next(L, 3);
        GLfloat *v = luabridge_check_vector(L, -1, 2);
        if (!v)
            luaL_error(L, "Invalid vec2 in point list (argument 2)");
        memcpy(&vertices[2*i], v, 2*sizeof(GLfloat));
        lua_pop(L, 1);
    }

    luaL_checktype(L, 4, LUA_TFUNCTION);
    lua_pushvalue(L, 4);
    int callback = luaL_ref(L, LUA_REGISTRYINDEX);

    engine_ptr e = luabridge_registry_get_engine(L);
    scene_add_trigger_region(sr->scene, pos, vertices, vertex_count, callback, e);
    free(vertices);
    return 0;
}

/*
 *  (void) scene:addTimeout(function callback, number timeout)
 */
static int add_timeout(lua_State *L)
{
    luabridge_sceneref *sr = luaL_checkudata(L, 1, LUABRIDGE_SCENE_TYPENAME);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_pushvalue(L, 2);
    int callback = luaL_ref(L, LUA_REGISTRYINDEX);
    GLfloat timeout = luaL_checknumber(L, 3);

    scene_add_timeout(sr->scene, callback, timeout);
    return 0;
}

/*
 *  (string) scene:__tostring(void)
 */
static int description(lua_State *L)
{
    luabridge_sceneref *sr = luaL_checkudata(L, 1, LUABRIDGE_SCENE_TYPENAME);
    lua_pushfstring(L, LUABRIDGE_SCENE_TYPENAME"(%p)", sr->scene);
    return 1;
}

void luabridge_register_sceneref(lua_State *L)
{
    const luaL_Reg methods[] = {
        {"getCameraOffset", get_camera_offset},
        {"setCameraOffset", set_camera_offset},
        {"loadActor", load_actor},
        {"loadLayer", load_layer},
        {"addTrigger", add_trigger},
        {"addTimeout", add_timeout},
        {"__tostring", description},
        {NULL, NULL}
    };

    // Register methods with the sceneref metatable
    luaL_newmetatable(L, LUABRIDGE_SCENE_TYPENAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);
}

/*
 * Run a trigger callback
 */
void luabridge_scene_run_trigger(lua_State *L, actor_ptr a, int callback)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
    luabridge_push_actorref(L, a);
    if (lua_pcall(L, 1, 0, 0))
    {
        fprintf(stderr, "Trigger callback failed.\n%s\n", lua_tostring(L, -1));
        assert(FATAL_ERROR);
    }
}

void luabridge_scene_run_timeout(lua_State *L, int callback, GLfloat overflow)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, callback);
    lua_pushnumber(L, overflow);
    if (lua_pcall(L, 1, 0, 0))
    {
        fprintf(stderr, "Timeout callback failed.\n%s\n", lua_tostring(L, -1));
        assert(FATAL_ERROR);
    }
}

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
#include "luabridge_actor.h"
#include "luabridge_vector.h"
#include "actor.h"

/*
 * Push an actor reference onto the lua stack
 */
void luabridge_push_actorref(lua_State *L, actor_ptr a)
{
    luabridge_actorref *ar = lua_newuserdata(L, sizeof(luabridge_actorref));
    luaL_setmetatable(L, LUABRIDGE_ACTOR_TYPENAME);
    ar->actor = a;
}

/*
 *  (void) actor:addToScene(vec3 position, number facing)
 */
static int add_to_scene(lua_State *L)
{
    luabridge_actorref *ar = luaL_checkudata(L, 1, LUABRIDGE_ACTOR_TYPENAME);
    GLfloat *pos = luabridge_check_vector(L, 2, 3);
    GLfloat facing = luaL_checknumber(L, 3);

    actor_add_to_walkmap(ar->actor, pos, facing, luabridge_registry_get_walkmap(L));
    return 0;
}

/*
 *  (void) actor:removeFromScene(void)
 */
static int remove_from_scene(lua_State *L)
{
    luabridge_actorref *ar = luaL_checkudata(L, 1, LUABRIDGE_ACTOR_TYPENAME);
    actor_remove_from_walkmap(ar->actor, luabridge_registry_get_walkmap(L));
    return 0;
}

/*
 *  (vec3) actor:getPosition(void)
 */
static int get_position(lua_State *L)
{
    luabridge_actorref *ar = luaL_checkudata(L, 1, LUABRIDGE_ACTOR_TYPENAME);

    GLfloat p[3];
    actor_position(ar->actor, p, luabridge_registry_get_walkmap(L));
    luabridge_push_vector(L, p, 3);
    return 1;
}

/*
 *  (void) actor:setPosition(vec3 position)
 */
static int set_position(lua_State *L)
{
    luabridge_actorref *ar = luaL_checkudata(L, 1, LUABRIDGE_ACTOR_TYPENAME);
    GLfloat *pos = luabridge_check_vector(L, 2, 3);
    actor_set_position(ar->actor, pos, luabridge_registry_get_walkmap(L));
    return 0;
}

/*
 *  (vec2) actor:getVelocity(void)
 */
static int get_velocity(lua_State *L)
{
    luabridge_actorref *ar = luaL_checkudata(L, 1, LUABRIDGE_ACTOR_TYPENAME);

    GLfloat v[2];
    actor_velocity(ar->actor, v, luabridge_registry_get_walkmap(L));
    luabridge_push_vector(L, v, 2);
    return 1;
}

/*
 *  (void) actor:getVelocity(vec2 velocity)
 */
static int set_velocity(lua_State *L)
{
    luabridge_actorref *ar = luaL_checkudata(L, 1, LUABRIDGE_ACTOR_TYPENAME);
    GLfloat *v = luabridge_check_vector(L, 2, 2);
    actor_set_velocity(ar->actor, v, luabridge_registry_get_walkmap(L));
    return 0;
}

/*
 *  (string) actor:__tostring(void)
 */
static int description(lua_State *L)
{
    luabridge_actorref *ar = luaL_checkudata(L, 1, LUABRIDGE_ACTOR_TYPENAME);
    lua_pushfstring(L, LUABRIDGE_ACTOR_TYPENAME"(%p)", ar->actor);
    return 1;
}

void luabridge_register_actorref(lua_State *L)
{
    const luaL_Reg methods[] = {
        {"addToScene", add_to_scene},
        {"removeFromScene", remove_from_scene},
        {"getPosition", get_position},
        {"setPosition", set_position},
        {"getVelocity", get_velocity},
        {"setVelocity", set_velocity},
        {"__tostring", description},
        {NULL, NULL}
    };

    // Register methods with the actorref metatable
    luaL_newmetatable(L, LUABRIDGE_ACTOR_TYPENAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, methods, 0);
}


#pragma mark Lua Scene Interface

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
#include "luabridge_layer.h"
#include "layer.h"

/*
 * Push a layer reference onto the lua stack
 */
void luabridge_push_layerref(lua_State *L, layer_ptr l)
{
    luabridge_layerref *lr = lua_newuserdata(L, sizeof(luabridge_layerref));
    luaL_setmetatable(L, LUABRIDGE_LAYER_TYPENAME);
    lr->layer = l;
}

/*
 *  (boolean) layer:getVisible()
 */
static int get_visible(lua_State *L)
{
    luabridge_layerref *lr = luaL_checkudata(L, 1, LUABRIDGE_LAYER_TYPENAME);
    lua_pushboolean(L, layer_visible(lr->layer));
    return 1;
}

/*
 *  (void) layer:setVisible(boolean visible)
 */
static int set_visible(lua_State *L)
{
    luabridge_layerref *lr = luaL_checkudata(L, 1, LUABRIDGE_LAYER_TYPENAME);

    // LuaL doesn't define a checkboolean type
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    layer_set_visible(lr->layer, lua_toboolean(L, 2));
    return 0;
}

/*
 *  (unsigned) layer:getFrame()
 */
static int get_frame(lua_State *L)
{
    luabridge_layerref *lr = luaL_checkudata(L, 1, LUABRIDGE_LAYER_TYPENAME);
    lua_pushunsigned(L, (lua_Unsigned)layer_frame(lr->layer));
    return 1;
}

/*
 *  (void) layer:setFrame(unsigned frame)
 */
static int set_frame(lua_State *L)
{
    luabridge_layerref *lr = luaL_checkudata(L, 1, LUABRIDGE_LAYER_TYPENAME);
    layer_set_frame(lr->layer, luaL_checkunsigned(L, 2));
    return 0;
}

/*
 *  (unsigned) layer:getFrameCount()
 */
static int get_framecount(lua_State *L)
{
    luabridge_layerref *lr = luaL_checkudata(L, 1, LUABRIDGE_LAYER_TYPENAME);
    lua_pushunsigned(L, (lua_Unsigned)layer_framecount(lr->layer));
    return 1;
}

/*
 *  (string) layer:__tostring(void)
 */
static int description(lua_State *L)
{
    luabridge_layerref *lr = luaL_checkudata(L, 1, LUABRIDGE_LAYER_TYPENAME);
    lua_pushfstring(L, LUABRIDGE_LAYER_TYPENAME"(%p)", lr->layer);
    return 1;
}

void luabridge_register_layerref(lua_State *L)
{
    const luaL_Reg methods[] = {
        {"getVisible", get_visible},
        {"setVisible", set_visible},
        {"getFrame", get_frame},
        {"setFrame", set_frame},
        {"getFrameCount", get_framecount},
        {"__tostring", description},
        {NULL, NULL}
    };

    // Register methods with the layerref metatable
    luaL_newmetatable(L, LUABRIDGE_LAYER_TYPENAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");

    luaL_setfuncs(L, methods, 0);
}

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
#include <string.h>

#include "luabridge.h"
#include "luabridge_vector.h"

// So we can do data manipulation with arrays on the stack
#define MAX_VEC_SIZE 4
#define LUABRIDGE_VECTOR_TYPENAME "vec"

struct vec
{
    uint8_t size;
    GLfloat data[MAX_VEC_SIZE];
};

static int vec2_new(lua_State *L)
{
    GLfloat v[] = {luaL_checknumber(L, 1), luaL_checknumber(L, 2)};
    luabridge_push_vector(L, v, 2);
    return 1;
}

static int vec3_new(lua_State *L)
{
    GLfloat v[] = {luaL_checknumber(L, 1), luaL_checknumber(L, 2), luaL_checknumber(L, 3)};
    luabridge_push_vector(L, v, 3);
    return 1;
}

static int vec4_new(lua_State *L)
{
    GLfloat v[] = {luaL_checknumber(L, 1), luaL_checknumber(L, 2),
                   luaL_checknumber(L, 3), luaL_checknumber(L, 4)};
    luabridge_push_vector(L, v, 4);
    return 1;
}

static int get(lua_State *L)
{
    struct vec *v = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);
    uint16_t i = luaL_checkunsigned(L, 2);
    if (i < 1 || i > v->size)
        luaL_error(L, "Attempting to access vec%d index %d:. Valid indices are 1-%d\n", v->size, i, v->size);

    lua_pushnumber(L, v->data[i-1]);
    return 1;
}

static int set(lua_State *L)
{
    struct vec *v = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);
    uint16_t i = luaL_checkunsigned(L, 2);
    if (i < 1 || i > v->size)
        luaL_error(L, "Attempting to access vec%d index %d:. Valid indices are 1-%d\n", v->size, i, v->size);

    v->data[i-1] = luaL_checknumber(L, 3);
    return 0;
}

static int unm(lua_State *L)
{
    struct vec *v = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);

    GLfloat new[MAX_VEC_SIZE];
    for (uint8_t i = 0; i < v->size; i++)
        new[i] = -v->data[i];

    luabridge_push_vector(L, new, v->size);
    return 1;
}

static int add(lua_State *L)
{
    struct vec *a = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);
    struct vec *b = luaL_checkudata(L, 2, LUABRIDGE_VECTOR_TYPENAME);
    if (a->size != b->size)
        luaL_error(L, "Unable to evaluate vec%d + vec%d.\n", a->size, b->size);

    GLfloat new[MAX_VEC_SIZE];
    for (uint8_t i = 0; i < a->size; i++)
        new[i] = a->data[i] + b->data[i];

    luabridge_push_vector(L, new, a->size);
    return 1;
}

static int sub(lua_State *L)
{
    struct vec *a = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);
    struct vec *b = luaL_checkudata(L, 2, LUABRIDGE_VECTOR_TYPENAME);
    if (a->size != b->size)
        luaL_error(L, "Unable to evaluate vec%d + vec%d.\n", a->size, b->size);

    GLfloat new[MAX_VEC_SIZE];
    for (uint8_t i = 0; i < a->size; i++)
        new[i] = a->data[i] - b->data[i];

    luabridge_push_vector(L, new, a->size);
    return 1;
}

static int mul(lua_State *L)
{
    // Accepts vec and number in either order
    struct vec *v;
    GLfloat mul;
    if (lua_isnumber(L, 1))
    {
        mul = luaL_checknumber(L, 1);
        v = luaL_checkudata(L, 2, LUABRIDGE_VECTOR_TYPENAME);
    }
    else
    {
        v = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);
        mul = luaL_checknumber(L, 2);
    }

    GLfloat new[MAX_VEC_SIZE];
    for (uint8_t i = 0; i < v->size; i++)
        new[i] = v->data[i]*mul;

    luabridge_push_vector(L, new, v->size);
    return 1;
}

static int div(lua_State *L)
{
    // Accepts vec and number in either order
    struct vec *v;
    GLfloat div;
    if (lua_isnumber(L, 1))
    {
        div = luaL_checknumber(L, 1);
        v = luaL_checkudata(L, 2, LUABRIDGE_VECTOR_TYPENAME);
    }
    else
    {
        v = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);
        div = luaL_checknumber(L, 2);
    }
    
    GLfloat new[MAX_VEC_SIZE];
    for (uint8_t i = 0; i < v->size; i++)
        new[i] = v->data[i]/div;
    
    luabridge_push_vector(L, new, v->size);
    return 1;
}

static int description(lua_State *L)
{
    struct vec *v = luaL_checkudata(L, 1, LUABRIDGE_VECTOR_TYPENAME);

    switch (v->size)
    {
        case 2: lua_pushfstring(L, "vec2(%f,%f)", v->data[0], v->data[1]); break;
        case 3: lua_pushfstring(L, "vec3(%f,%f,%f)", v->data[0], v->data[1], v->data[2]); break;
        case 4: lua_pushfstring(L, "vec4(%f,%f,%f,%f)", v->data[0], v->data[1], v->data[2], v->data[3]); break;
        default: printf("Unknown vector size: %d.\n", v->size); assert(FATAL_ERROR); break;
    }
    return 1;
}

void luabridge_register_vectors(lua_State *L)
{
    const luaL_Reg methods[] = {
        {"__tostring", description},
        {"get", get},
        {"__index", get},
        {"__newindex", set},
        {"__unm", unm},
        {"__add", add},
        {"__sub", sub},
        {"__mul", mul},
        {"__div", div},
        {NULL, NULL}
    };

    luaL_newmetatable(L, LUABRIDGE_VECTOR_TYPENAME);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, methods, 0);

    lua_register(L, LUABRIDGE_VECTOR_TYPENAME"2", vec2_new);
    lua_register(L, LUABRIDGE_VECTOR_TYPENAME"3", vec3_new);
    lua_register(L, LUABRIDGE_VECTOR_TYPENAME"4", vec4_new);
}

void luabridge_push_vector(lua_State *L, GLfloat *data, uint8_t n)
{
    assert(n >= 2 && n <= MAX_VEC_SIZE);
    struct vec *v = lua_newuserdata(L, sizeof(struct vec) + (n-1)*sizeof(GLfloat));
    memcpy(v->data, data, n*sizeof(GLfloat));
    v->size = n;
    luaL_setmetatable(L, LUABRIDGE_VECTOR_TYPENAME);
}

GLfloat *luabridge_check_vector(lua_State *L, int ud, uint8_t n)
{
    char msg[32];
    struct vec *v = luaL_testudata(L, ud, LUABRIDGE_VECTOR_TYPENAME);
    if (!v)
    {
        snprintf(msg, 32, "vec%d expected, got %s", n, luaL_typename(L, ud));
        luaL_argerror(L, ud, msg);
    }

    if (v->size != n)
    {
        snprintf(msg, 32, "vec%d expected, got vec%d", n, v->size);
        luaL_argerror(L, ud, msg);
    }
    return v->data;
}

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

#ifndef GamePrototype_typedefs_h
#define GamePrototype_typedefs_h

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#if PLATFORM_GLES
    #import <OpenGLES/ES2/gl.h>
    #import <OpenGLES/ES2/glext.h>

    #define glBindVertexArray glBindVertexArrayOES
    #define glGenVertexArrays glGenVertexArraysOES
    #define glDeleteVertexArrays glDeleteVertexArraysOES
#else
    #import <OpenGL/OpenGL.h>
    #import <OpenGL/gl3.h>
#endif

// Defines for assertions
#define NOT_IMPLEMENTED false
#define FATAL_ERROR false

typedef struct
{
    GLfloat radius;
    GLfloat angle;
} GPpolar;

// Ensure this is big enough to hold the input flags
typedef enum
{
    INPUT_RESET_CAMERA = (1 << 0),
} input_flag_types;
typedef uint8_t input_flags;

typedef enum
{
    ANALOG_INPUT_DIRECTION = 0,
    ANALOG_INPUT_CAMERA    = 1,
} analog_input_type;


typedef struct
{
    GLuint texture;
    GLfloat width;
    GLfloat height;
} textureref;

typedef struct texture *texture_ptr;

// Shared texture managed by the engine
// Can only be bound by callers
typedef struct texture const *texture_instance_ptr;

typedef struct engine *engine_ptr;
typedef struct renderer *renderer_ptr;
typedef struct modelview *modelview_ptr;
typedef struct framebuffer *framebuffer_ptr;
typedef struct scene *scene_ptr;
typedef struct layer *layer_ptr;
typedef struct walkmap *walkmap_ptr;
typedef struct walkmap_actordata *walkmap_actordata_ptr;
typedef struct actor *actor_ptr;
typedef struct model *model_ptr;
typedef struct font *font_ptr;
typedef struct font const *font_instance_ptr;

typedef struct widget *widget_ptr;
typedef struct widget_string *widget_string_ptr;

typedef struct frame *frame_ptr;
typedef struct vertexarray *vertexarray_ptr;

// Defined in engine.h
typedef struct engine_config *engine_config_ptr;

// Defined in frame.h
typedef struct transition_instance *transition_instance_ptr;

#endif

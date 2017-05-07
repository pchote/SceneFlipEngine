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

#ifndef GamePrototype_renderer_h
#define GamePrototype_renderer_h

#include "typedefs.h"

#define checkGLError()									\
{														\
    GLenum err = glGetError();							\
    bool failed = false;                                \
    while (err != GL_NO_ERROR) {						\
        failed = true;                                  \
        fprintf(stderr, "GLError %s set in File:%s Line:%d\n",	\
                GetGLErrorString(err),					\
                __FILE__,								\
                __LINE__);								\
        err = glGetError();								\
    }													\
    assert(!failed);                                    \
}

static inline const char * GetGLErrorString(GLenum error)
{
	const char *str;
	switch( error )
	{
		case GL_NO_ERROR:
			str = "GL_NO_ERROR";
			break;
		case GL_INVALID_ENUM:
			str = "GL_INVALID_ENUM";
			break;
		case GL_INVALID_VALUE:
			str = "GL_INVALID_VALUE";
			break;
		case GL_INVALID_OPERATION:
			str = "GL_INVALID_OPERATION";
			break;
#if defined __gl_h_ || defined __gl3_h_
		case GL_OUT_OF_MEMORY:
			str = "GL_OUT_OF_MEMORY";
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			str = "GL_INVALID_FRAMEBUFFER_OPERATION";
			break;
#endif
#if defined __gl_h_
		case GL_STACK_OVERFLOW:
			str = "GL_STACK_OVERFLOW";
			break;
		case GL_STACK_UNDERFLOW:
			str = "GL_STACK_UNDERFLOW";
			break;
		case GL_TABLE_TOO_LARGE:
			str = "GL_TABLE_TOO_LARGE";
			break;
#endif
		default:
			str = "(ERROR: Unknown Error Enum)";
			break;
	}
	return str;
}


// List of vertex attribute types for shaders
enum
{
	VERTEX_POS_ATTRIB_IDX,
	TEXTURE_COORDS_ATTRIB_IDX,
    COLOR_ATTRIB_IDX
};

renderer_ptr renderer_create();
void renderer_destroy(renderer_ptr r);
void renderer_enable_layer_shader(renderer_ptr r, GLfloat mvp[16]);
void renderer_enable_model_shader(renderer_ptr r, GLfloat mvp[16]);
void renderer_enable_text_shader(renderer_ptr r, GLfloat mvp[16]);
void renderer_enable_line_shader(renderer_ptr r, GLfloat mvp[16], GLfloat color[4]);
void renderer_enable_line_color_shader(renderer_ptr r, GLfloat mvp[16]);
void renderer_enable_transition_shader(renderer_ptr r, GLfloat mvp[16], double dt);

#endif

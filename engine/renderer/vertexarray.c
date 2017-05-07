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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "engine.h"
#include "renderer.h"
#include "vertexarray.h"

struct vertexarray
{
    GLuint vao;
    GLuint vbo;
    GLuint tbo;
    GLenum type;
    GLsizei vertex_count;
    GLsizei texcoord_size;

    // For delayed init
    bool initialized;
    GLfloat *vertices;
    GLfloat *texcoords;
};

/*
 * Initialize the vertex array gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void init_gl(void *_va)
{
    vertexarray_ptr va = _va;
    if (va->initialized)
    {
        // May be called by draw before engine runs the task
        printf("Attempting to initialize already initialized vertex array.\n");
        return;
    }

    glGenVertexArrays(1, &va->vao); checkGLError();
    glGenBuffers(1, &va->vbo); checkGLError();
    glGenBuffers(1, &va->tbo); checkGLError();
    assert(va->vao && va->vbo && va->tbo);

    glBindVertexArray(va->vao); checkGLError();

    // Fill vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, va->vbo); checkGLError();
    glBufferData(GL_ARRAY_BUFFER, 3*va->vertex_count*sizeof(GLfloat), va->vertices, GL_STATIC_DRAW); checkGLError();
    glVertexAttribPointer(VERTEX_POS_ATTRIB_IDX, 3, GL_FLOAT, GL_FALSE, 0, 0); checkGLError();
    glEnableVertexAttribArray(VERTEX_POS_ATTRIB_IDX); checkGLError();

    // Fill texcoord buffer
    if (va->texcoord_size > 0)
    {
        glBindBuffer(GL_ARRAY_BUFFER, va->tbo); checkGLError();
        glBufferData(GL_ARRAY_BUFFER, va->texcoord_size*va->vertex_count*sizeof(GLfloat), va->texcoords, GL_STATIC_DRAW); checkGLError();
        glVertexAttribPointer(TEXTURE_COORDS_ATTRIB_IDX, va->texcoord_size, GL_FLOAT, GL_FALSE, 0, 0); checkGLError();
        glEnableVertexAttribArray(TEXTURE_COORDS_ATTRIB_IDX); checkGLError();
    }

    glBindVertexArray(0);
    free(va->vertices);
    free(va->texcoords);
    va->initialized = true;
}

/*
 * Uninitialize the vertex array gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void uninit_gl(void *_va)
{
    vertexarray_ptr va = _va;
    assert(va->initialized);

    glDeleteBuffers(1, &va->vbo); checkGLError();
    glDeleteBuffers(1, &va->tbo); checkGLError();
	glDeleteVertexArrays(1, &va->vao); checkGLError();
    free(va);
}

/*
 * Create a vertexarray (vao) with the given vertices, texcoords, and draw type
 * Note: vertices and texcoords may be null if the caller wants to allocate space
 *       to update with later calls
 *
 * Call Context: Main thread
 */
vertexarray_ptr vertexarray_create(GLfloat *vertices, GLfloat *texcoords, GLsizei vertex_count, GLsizei texcoord_size, GLenum type, engine_ptr e)
{
    vertexarray_ptr va = calloc(1, sizeof(struct vertexarray));
    assert(va);

    va->vertex_count = vertex_count;
    va->type = type;
    va->texcoord_size = texcoord_size;

    if (vertices)
    {
        size_t s = 3*va->vertex_count*sizeof(GLfloat);
        va->vertices = malloc(s);
        assert(va->vertices);
        memcpy(va->vertices, vertices, s);
    }

    if (texcoords)
    {
        size_t s = va->texcoord_size*va->vertex_count*sizeof(GLfloat);
        va->texcoords = malloc(s);
        assert(va->texcoords);
        memcpy(va->texcoords, texcoords, s);
    }

    engine_queue_task(e, init_gl, va);
    return va;
}

/*
 * Special case vertexarray, representing a quad
 *
 * Call Context: Main thread
 */
vertexarray_ptr vertexarray_create_quad(GLfloat width, GLfloat height, engine_ptr e)
{
    GLfloat w = width/height;
    GLfloat h = 1;
    GLfloat vertices[] =
    {
        w, h, 0,
        -w, h, 0,
        w,-h, 0,
        -w,-h, 0
    };

    GLfloat texcoords[] =
    {
        width, height,
        0, height,
        width, 0,
        0, 0
    };

    return vertexarray_create(vertices, texcoords, 4, 2, GL_TRIANGLE_STRIP, e);
}

/*
 * Free the resources associated with a vertexarray object
 *
 * Call Context: Main thread
 */
void vertexarray_destroy(vertexarray_ptr va, engine_ptr e)
{
    engine_queue_task(e, uninit_gl, va);
}

/*
 * Update the vertex or texcoord arrays with new data
 *
 * Call Context: Main thread
 */
void vertexarray_update(vertexarray_ptr va, GLfloat *vertices, GLfloat *texcoords, GLsizei count, GLenum type)
{
    if (!va->initialized)
    {
        printf("WARNING: Attempting to access uninitialized vertexarray. Initializing on hot path.\n");
        init_gl(va);
    }

    va->vertex_count = count;
    glBindVertexArray(va->vao); checkGLError();
    if (vertices)
    {
        glBindBuffer(GL_ARRAY_BUFFER, va->vbo); checkGLError();
        glBufferData(GL_ARRAY_BUFFER, 3*va->vertex_count*sizeof(GLfloat), vertices, type); checkGLError();
    }

    if (texcoords)
    {
        glBindBuffer(GL_ARRAY_BUFFER, va->tbo); checkGLError();
        glBufferData(GL_ARRAY_BUFFER, va->texcoord_size*va->vertex_count*sizeof(GLfloat), texcoords, type); checkGLError();
    }

    glBindVertexArray(0);
}

/*
 * Draw the vertexarray
 *
 * Call Context: Main thread
 */
void vertexarray_draw(vertexarray_ptr va)
{
    if (!va->initialized)
    {
        printf("WARNING: Attempting to access uninitialized vertexarray. Initializing on hot path.\n");
        init_gl(va);
    }

    glBindVertexArray(va->vao); checkGLError();
    glDrawArrays(va->type, 0, va->vertex_count); checkGLError();
    glBindVertexArray(0); checkGLError();
}

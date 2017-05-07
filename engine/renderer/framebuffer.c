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
#include <string.h>
#include <stdlib.h>
#include "engine.h"
#include "renderer.h"
#include "framebuffer.h"

/*
 * Private implementation details
 */
struct framebuffer
{
    // GL references to framebuffer, texture, depth buffer objects
    GLuint fbo;
    GLuint texture;
    GLuint depth;

    // Actual size (2^n, square) of framebuffer
    GLuint size;

    // Size of renderable subregion
    GLuint width;
    GLuint height;

    // Reference to previous fbo to restore state
    GLint previous_fbo;

    bool initialized;
};

#define MAX(a,b) (((a)>(b))?(a):(b))

// Calculate the next largest power of two for a 32bit number x
static inline GLuint npot(GLuint x)
{
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x + 1;
}

/*
 * Calculate the size of the backend texture
 * used for a given renderable area
 */
GLuint framebuffer_size(GLuint width, GLuint height)
{
    return npot(MAX(width, height));
}

/*
 * Initialize the framebuffer gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void init_gl(void *_fb)
{
    framebuffer_ptr fb = _fb;
    if (fb->initialized)
    {
        // May be called by draw before engine runs the task
        printf("Attempting to initialize already initialized framebuffer array.\n");
        return;
    }

    // Save current buffer
    GLint current;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current);

    glGenFramebuffers(1, &fb->fbo); checkGLError();
    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo); checkGLError();

    // Color buffer
    glGenTextures(1, &fb->texture); checkGLError();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fb->texture); checkGLError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,  fb->size, fb->size, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); checkGLError();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb->texture, 0); checkGLError();

    // Depth buffer
    glGenRenderbuffers(1, &fb->depth); checkGLError();
    glBindRenderbuffer(GL_RENDERBUFFER, fb->depth); checkGLError();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, fb->size, fb->size); checkGLError();

    // Test for completeness
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fb->depth); checkGLError();
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER); checkGLError();
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    // Restore original buffer
    glBindFramebuffer(GL_FRAMEBUFFER, current); checkGLError();

    fb->initialized = true;
}

/*
 * Uninitialize the framebuffer gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void uninit_gl(void *_fb)
{
    framebuffer_ptr fb = _fb;
    assert(fb->initialized);

    glDeleteTextures(1, &fb->texture);
    glDeleteRenderbuffers(1, &fb->depth);
    glDeleteFramebuffers(1, &fb->fbo);
    free(fb);
}

/*
 * Create a framebuffer object, with a square texture of the given size
 */
framebuffer_ptr framebuffer_create(GLuint width, GLuint height, engine_ptr e)
{
    framebuffer_ptr fb = calloc(1, sizeof(struct framebuffer));
    assert(fb);

    // TODO: Ensure these are within GL_MAX_VIEWPORT_DIMS
    fb->width = width;
    fb->height = height;
    fb->size = framebuffer_size(width, height);

    engine_queue_task(e, init_gl, fb);
    return fb;
}

/*
 * Destroy framebuffer
 */
void framebuffer_destroy(framebuffer_ptr fb, engine_ptr e)
{
    engine_queue_task(e, uninit_gl, fb);
}

/*
 * Bind the framebuffer to the current context,
 * and prepare a viewport with given dimensions.
 * The previous framebuffer is stored so it can
 * be restored in framebuffer_unbind.
 *
 * Call Context: Main thread
 */
void framebuffer_bind(framebuffer_ptr fb)
{
    if (!fb->initialized)
    {
        printf("WARNING: Attempting to access uninitialized framebuffer. Initializing on hot path.\n");
        init_gl(fb);
    }

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fb->previous_fbo);

    glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);
    glViewport(0, 0, fb->width, fb->height); checkGLError();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/*
 * Flush the rendering pipeline and restore
 * the previous framebuffer
 */
void framebuffer_unbind(framebuffer_ptr fb)
{
    glFlush();

    // Restore the previous buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fb->previous_fbo);
}

/*
 * Return a texture reference that can be rendered on external geometry
 * Width and height define the area within the texture that contain
 * the rendered viewport
 */

// TODO: This is shit
textureref framebuffer_get_textureref(framebuffer_ptr fb)
{
    return (textureref)
    {
        .texture = fb->texture,
        .width = fb->width*1.0f/fb->size,
        .height = fb->height*1.0f/fb->size
    };
}


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

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "widget_string.h"
#include "widget.h"
#include "font.h"
#include "engine.h"
#include "renderer.h"
#include "modelview.h"

struct widget_string
{
    char *text;
    bool dirty;
    font_instance_ptr font_ref;

    GLenum lifetime;
    GLuint vao;
    GLuint vbo;
    GLsizei vertex_count;
    bool initialized;
};

/*
 * Initialize the vertex array gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void init_gl(void *_ws)
{
    widget_string_ptr ws = _ws;
    if (ws->initialized)
    {
        // May be called by draw before engine runs the task
        printf("Attempting to initialize already initialized widget_string.\n");
        return;
    }

    glGenVertexArrays(1, &ws->vao); checkGLError();
    glGenBuffers(1, &ws->vbo); checkGLError();
    assert(ws->vao && ws->vbo);
    glBindVertexArray(ws->vao); checkGLError();

    // Vertex buffer
    GLsizei stride = 9*sizeof(GLfloat);
    glBindBuffer(GL_ARRAY_BUFFER, ws->vbo); checkGLError();
    glVertexAttribPointer(VERTEX_POS_ATTRIB_IDX, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid *)0); checkGLError();
    glEnableVertexAttribArray(VERTEX_POS_ATTRIB_IDX); checkGLError();

    // Texcoord buffer
    glVertexAttribPointer(TEXTURE_COORDS_ATTRIB_IDX, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid *)(3*sizeof(GLfloat))); checkGLError();
    glEnableVertexAttribArray(TEXTURE_COORDS_ATTRIB_IDX); checkGLError();

    // Color buffer
    glVertexAttribPointer(COLOR_ATTRIB_IDX, 4, GL_FLOAT, GL_FALSE, stride, (GLvoid *)(5*sizeof(GLfloat))); checkGLError();
    glEnableVertexAttribArray(COLOR_ATTRIB_IDX); checkGLError();

    // Unbind array
    glBindVertexArray(0);

    ws->initialized = true;
}

/*
 * Uninitialize the vertex array gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void uninit_gl(void *_ws)
{
    widget_string_ptr ws = _ws;
    assert(ws->initialized);

    glDeleteBuffers(1, &ws->vbo); checkGLError();
	glDeleteVertexArrays(1, &ws->vao); checkGLError();

    free(ws->text);
    free(ws);
}

widget_string_ptr widget_string_create(const char *font_id, engine_ptr e)
{
    widget_string_ptr ws = calloc(1, sizeof(struct widget_string));
    assert(ws);

    ws->lifetime = GL_STATIC_DRAW;
    ws->font_ref = engine_retain_font(e, font_id);

    engine_queue_task(e, init_gl, ws);

    return ws;
}

void widget_string_destroy(widget_string_ptr ws, engine_ptr e)
{
    engine_release_font(e, ws->font_ref);
    engine_queue_task(e, uninit_gl, ws);
}

static void update_buffers(widget_string_ptr ws)
{
    // Update vertex array
    GLsizei text_len = font_string_glyph_count(ws->font_ref, ws->text);
    ws->vertex_count = 6*text_len;
    GLsizei bsize = 54*text_len*sizeof(GLfloat);

    GLfloat *buffer = malloc(bsize);
    assert(buffer);
    font_render_string(ws->font_ref, ws->text, text_len, buffer);
    glBindBuffer(GL_ARRAY_BUFFER, ws->vbo); checkGLError();
    glBufferData(GL_ARRAY_BUFFER, bsize, buffer, ws->lifetime); checkGLError();
    free(buffer);

    ws->dirty = false;
}

void widget_string_draw(widget_string_ptr ws, modelview_ptr mv, renderer_ptr r)
{
    if (!ws->text)
        return;

    if (!ws->initialized)
    {
        printf("WARNING: Attempting to access uninitialized widget_string. Initializing on hot path.\n");
        init_gl(ws);
    }

    if (ws->dirty)
        update_buffers(ws);

    GLfloat mvp[16];
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_text_shader(r, mvp);
    glBindVertexArray(ws->vao); checkGLError();

    font_bind_texture(ws->font_ref);
    glDrawArrays(GL_TRIANGLES, 0, ws->vertex_count); checkGLError();
    glBindVertexArray(0); checkGLError();
}

void widget_string_debug_draw(widget_string_ptr ws, modelview_ptr mv, renderer_ptr r)
{
    if (!ws->text)
        return;

    if (!ws->initialized)
    {
        printf("WARNING: Attempting to access uninitialized widget_string. Initializing on hot path.\n");
        init_gl(ws);
    }

    if (ws->dirty)
        update_buffers(ws);

    GLfloat mvp[16];
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_line_color_shader(r, mvp);
    glBindVertexArray(ws->vao); checkGLError();

#if !PLATFORM_GLES
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, ws->vertex_count); checkGLError();
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

    glBindVertexArray(0); checkGLError();
}

void widget_string_set_text(widget_string_ptr ws, char *text, GLenum lifetime)
{
    ws->dirty = true;

    free(ws->text);
    ws->text = strdup(text);
    assert(ws->text);
}

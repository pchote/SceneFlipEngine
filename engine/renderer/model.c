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
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "typedefs.h"
#include "engine.h"
#include "renderer.h"
#include "texture.h"
#include "model.h"
#include "vertexarray.h"

#define LERP(x,y,t) ((x)+(t)*(y - x))

/*
 * Private implementation details
 */
struct model_header
{
    uint32_t version;
    uint32_t frame_count;
    uint32_t triangle_count;
    uint32_t texture_name_length;
};

struct model
{
    GLfloat *vertex_data;
    GLfloat *texcoord_data;
    GLsizei vertex_count;
    GLsizei frame_count;

    GLfloat *current_vertex_data;
    GLfloat current_animation_fraction;
    texture_instance_ptr texture;

    vertexarray_ptr va;
    bool gl_dirty;
};

/*
 * Load a model from a binary .mdl file
 *
 * Call Context: Worker thread
 */
model_ptr model_create(const char *path, engine_ptr e)
{
    model_ptr m = calloc(1, sizeof(struct model));
    assert(m);

    // Open file
    FILE *mdl = fopen(path, "rb");
    assert(mdl);

    // Load header
    struct model_header h;
    fread(&h, sizeof(struct model_header), 1, mdl);
    assert(h.version == 1);

    m->frame_count = h.frame_count;
    m->vertex_count = 3*h.triangle_count;

    // Allocate arrays
    m->vertex_data = calloc(3*m->frame_count*m->vertex_count, sizeof(GLfloat));
    assert(m->vertex_data);

    m->texcoord_data = calloc(2*m->vertex_count, sizeof(GLfloat));
    assert(m->texcoord_data);

    char *texture_name = calloc(h.texture_name_length+1, sizeof(char));
    assert(texture_name);

    // Load Data
    fread(m->vertex_data, sizeof(GLfloat), 3*m->frame_count*m->vertex_count, mdl);
    fread(m->texcoord_data, sizeof(GLfloat), 2*m->vertex_count, mdl);
    fread(texture_name, sizeof(char), h.texture_name_length, mdl);
    texture_name[h.texture_name_length] = '\0';
    fclose(mdl);

    // Working set for the current animation state
    m->current_vertex_data = calloc(3*m->vertex_count, sizeof(GLfloat));
    assert(m->current_vertex_data);

    model_set_animation_frac(m, 0);

    m->va = vertexarray_create(NULL, m->texcoord_data, m->vertex_count, 2, GL_TRIANGLES, e);
    m->texture = engine_retain_texture(e, texture_name);
    free(texture_name);
    return m;
}

/*
 * Destroy model
 */
void model_destroy(model_ptr m, engine_ptr e)
{
    vertexarray_destroy(m->va, e);
    engine_release_texture(e, m->texture);

    free(m->current_vertex_data);
    free(m->texcoord_data);
    free(m->vertex_data);
    free(m);
}

/*
 * Render a model in the current GL context
 *
 * Call Context: Main thread
 */
void model_draw(model_ptr m, renderer_ptr r)
{
    if (m->gl_dirty)
    {
        vertexarray_update(m->va, m->current_vertex_data, NULL, m->vertex_count, GL_STREAM_DRAW);
        m->gl_dirty = false;
    }

    texture_bind(m->texture, GL_TEXTURE0);
    vertexarray_draw(m->va);
}

/*
 * Fill the currently bound vbo with vertex data
 * given by the current animation fraction
 *
 * Call Context: Main thread / Worker thread
 */
void model_set_animation_frac(model_ptr m, GLfloat frac)
{
    assert(frac >= 0);
    assert(frac <= 1);
    m->current_animation_fraction = frac;

    GLfloat frame_progress = frac*(m->frame_count-1);
    size_t prev_frame = floor(frame_progress);
    GLfloat t = frame_progress - prev_frame;

    // Lerp between frames
    size_t prev_index = prev_frame*3*m->vertex_count;
    size_t next_index = prev_index + 3*m->vertex_count;

    for (size_t i = 0; i < 3*m->vertex_count; i++)
        m->current_vertex_data[i] = LERP(m->vertex_data[prev_index + i], m->vertex_data[next_index + i], t);

    m->gl_dirty = true;
}

/*
 * Advance the animation progress by the requested amount
 *
 * Call Context: Main thread / Worker thread
 */
void model_step_animation_frac(model_ptr m, GLfloat frac)
{
    GLfloat new = m->current_animation_fraction + frac;
    new = fmod(new, 1.0);
    if (new < 0)
        new += 1.0;
    model_set_animation_frac(m, new);
}

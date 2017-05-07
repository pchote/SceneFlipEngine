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
#include "renderer.h"
#include "modelview.h"
#include "vertexarray.h"
#include "framebuffer.h"
#include "renderer.h"
#include "transition_instance.h"
#include "transition_fade.h"

// 2 second transition
#define TRANSITION_TIME 0.5

struct transition_fade_extra
{
    double time;
};

void transition_fade_initialize(transition_instance_ptr ti, renderer_ptr r)
{
    // Can only fade between matching sized textures that sizes match
    assert(ti->to_ref->width == ti->from_ref->width);
    assert(ti->to_ref->height == ti->from_ref->height);

    struct transition_fade_extra *extra = calloc(1, sizeof(struct transition_fade_extra));
    assert(extra);

    extra->time = 0;
    ti->extra = extra;
}

void transition_fade_cleanup(transition_instance_ptr ti, engine_ptr e)
{
    free(ti->extra);
}

bool transition_fade_tick(transition_instance_ptr ti, double dt, engine_ptr e, renderer_ptr r)
{
    struct transition_fade_extra *extra = ti->extra;
    extra->time += dt;
    if (extra->time >= TRANSITION_TIME)
        extra->time = TRANSITION_TIME;

    return ti->loaded && extra->time == TRANSITION_TIME;
}

void transition_fade_draw(transition_instance_ptr ti, modelview_ptr mv, renderer_ptr r)
{
    GLfloat mvp[16];
    struct transition_fade_extra *extra = ti->extra;
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_transition_shader(r, mvp, extra->time/TRANSITION_TIME);

    // Bind Textures, draw, cleanup
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ti->to_ref->texture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ti->from_ref->texture);
    vertexarray_draw(ti->quad_ref);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

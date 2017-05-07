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
#include "matrix.h"
#include "vertexarray.h"
#include "framebuffer.h"
#include "renderer.h"
#include "transition_instance.h"
#include "transition_slide.h"

// 2 second transition
#define TRANSITION_TIME 0.5

struct transition_slide_extra
{
    double time;
    GLfloat width;
    GLfloat dx;
};

void transition_slide_initialize(transition_instance_ptr ti, renderer_ptr r)
{
    // Can only slide between matching sized textures that sizes match
    assert(ti->to_ref->width == ti->from_ref->width);
    assert(ti->to_ref->height == ti->from_ref->height);

    struct transition_slide_extra *extra = calloc(1, sizeof(struct transition_slide_extra));
    assert(extra);

    extra->time = 0;
    extra->dx = 0;
    extra->width = 2.667;
    ti->extra = extra;
}

void transition_slide_cleanup(transition_instance_ptr ti, engine_ptr e)
{
    free(ti->extra);
}

bool transition_slide_tick(transition_instance_ptr ti, double dt, engine_ptr e, renderer_ptr r)
{
    struct transition_slide_extra *extra = ti->extra;
    extra->time += dt;
    if (extra->time >= TRANSITION_TIME)
        extra->time = TRANSITION_TIME;

    extra->dx = extra->width*extra->time/TRANSITION_TIME;
    return ti->loaded && extra->time == TRANSITION_TIME;
}

void transition_slide_draw(transition_instance_ptr ti, modelview_ptr mv, renderer_ptr r)
{
    GLfloat mvp[16];
    GLfloat *modelview = modelview_push(mv);
    struct transition_slide_extra *extra = ti->extra;

    mtxTranslateApply(modelview, -extra->dx, 0, 0);
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_model_shader(r, mvp);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ti->from_ref->texture);
    vertexarray_draw(ti->quad_ref);

    mtxTranslateApply(modelview, extra->width, 0, 0);
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_model_shader(r, mvp);
    glBindTexture(GL_TEXTURE_2D, ti->to_ref->texture);
    vertexarray_draw(ti->quad_ref);
    glBindTexture(GL_TEXTURE_2D, 0);
    modelview_pop(mv);
}

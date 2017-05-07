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
#include "renderer.h"
#include "vertexarray.h"
#include "modelview.h"
#include "transition_instance.h"
#include "transition_instant.h"

void transition_instant_initialize(transition_instance_ptr ti, renderer_ptr r)
{
    // Can only fade between matching sized textures that sizes match
    assert(ti->to_ref->width == ti->from_ref->width);
    assert(ti->to_ref->height == ti->from_ref->height);
}

bool transition_instant_tick(transition_instance_ptr ti, double dt, engine_ptr e, renderer_ptr r)
{
    return ti->loaded;
}

void transition_instant_cleanup(transition_instance_ptr ti, engine_ptr e) {}

void transition_instant_draw(transition_instance_ptr ti, modelview_ptr mv, renderer_ptr r)
{
    // Render the scene preview while we wait for it to load
    GLfloat mvp[16];
    modelview_calculate_mvp(mv, mvp);

    renderer_enable_model_shader(r, mvp);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ti->to_ref->texture); checkGLError();
    vertexarray_draw(ti->quad_ref);
}

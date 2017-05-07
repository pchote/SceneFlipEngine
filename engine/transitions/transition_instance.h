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

#ifndef GPEngine_transition_instance_h
#define GPEngine_transition_instance_h

#include "typedefs.h"

struct transition_type
{
    const char *name;
    void (*initialize)(transition_instance_ptr ti, renderer_ptr r);
    void (*cleanup)(transition_instance_ptr ti, engine_ptr e);
    bool (*tick)(transition_instance_ptr ti, double dt, engine_ptr e, renderer_ptr r);
    void (*draw)(transition_instance_ptr ti, modelview_ptr mv, renderer_ptr r);
};

struct transition_instance
{
    textureref *from_ref;
    textureref *to_ref;
    vertexarray_ptr quad_ref;
    bool loaded;
    void *extra;
    const struct transition_type *type;
};

transition_instance_ptr transition_instance_create(const char *type, vertexarray_ptr quad, textureref *from, textureref *to, renderer_ptr r);
void transition_instance_destroy(transition_instance_ptr ti, engine_ptr e);

#endif

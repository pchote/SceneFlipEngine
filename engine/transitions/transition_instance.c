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
#include "framebuffer.h"
#include "scene.h"
#include "vertexarray.h"

#include "transition_instance.h"
#include "transition_fade.h"
#include "transition_slide.h"
#include "transition_instant.h"
#include "transition_startup.h"

/*
 * Private implementation details
 */
const static struct transition_type transition_types[] = 
{
    {
        .name = "fade",
        .initialize = transition_fade_initialize,
        .cleanup = transition_fade_cleanup,
        .tick = transition_fade_tick,
        .draw = transition_fade_draw
    },
    {
        .name = "slide",
        .initialize = transition_slide_initialize,
        .cleanup = transition_slide_cleanup,
        .tick = transition_slide_tick,
        .draw = transition_slide_draw
    },
    {
        .name = "instant",
        .initialize = transition_instant_initialize,
        .cleanup = transition_instant_cleanup,
        .tick = transition_instant_tick,
        .draw = transition_instant_draw
    },
    {
        .name = "startup",
        .initialize = transition_startup_initialize,
        .cleanup = transition_startup_cleanup,
        .tick = transition_startup_tick,
        .draw = transition_startup_draw
    },
};
static size_t transition_count = 4;

void transition_instance_destroy(transition_instance_ptr ti, engine_ptr e)
{
    ti->type->cleanup(ti, e);
    free(ti);
}

transition_instance_ptr transition_instance_create(const char *type, vertexarray_ptr quad, textureref *from, textureref *to, renderer_ptr r)
{
    // Search for the requested transition
    for (size_t i = 0; i < transition_count; i++)
    {
        if (strcmp(type, transition_types[i].name) == 0)
        {
            transition_instance_ptr ti = calloc(1, sizeof(struct transition_instance));
            ti->from_ref = from;
            ti->to_ref = to;
            ti->quad_ref = quad;
            ti->type = &transition_types[i];
            ti->type->initialize(ti, r);
            return ti;
        }
    }
    assert(false);
    return NULL;
}

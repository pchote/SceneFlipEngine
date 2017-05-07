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
#include <math.h>

#include "renderer.h"
#include "matrix.h"
#include "modelview.h"
#include "scene.h"
#include "walkmap.h"
#include "actor.h"
#include "model.h"

/*
 * Private implementation detail
 */
struct actor
{
    GLfloat facing;
    GLfloat collision_radius;
    GLfloat cached_position[3];
    walkmap_actordata_ptr walkmap_data;
    model_ptr model;
};

/*
 * Called by the walkmap when the actor moves
 */
void actor_movement_callback(actor_ptr a, GLfloat new_pos[3], GLfloat old_pos[3])
{
    // Update graphics
    GLfloat dx = new_pos[0] - old_pos[0];
    GLfloat dy = new_pos[1] - old_pos[1];
    GLfloat moved = sqrt(dx*dx+dy*dy);
    if (moved > 0.01)
    {
        // Set facing based on actual movement vector
        a->facing = atan2f(dy, dx)*180/M_PI + 90;
        model_step_animation_frac(a->model, 0.5*moved);
    }

    a->cached_position[0] = new_pos[0];
    a->cached_position[1] = new_pos[1];
    a->cached_position[2] = new_pos[2];
}

/*
 * Create a new actor from the given definition
 *
 * Call Context: Worker thread
 */
actor_ptr actor_create(const char *model, GLfloat collision_radius, walkmap_ptr w, engine_ptr e)
{
    actor_ptr a = calloc(1, sizeof(struct actor));
    if (!a)
        return NULL;

    a->collision_radius = collision_radius;
    a->model = model_create(model, e);
    return a;
}

/*
 * Destroy actor
 *
 * Call Context: Main thread
 */
void actor_destroy(actor_ptr a, walkmap_ptr w, engine_ptr e)
{
    model_destroy(a->model, e);
    if (a->walkmap_data)
        actor_remove_from_walkmap(a, w);

    free(a);
}

void actor_add_to_walkmap(actor_ptr a, GLfloat pos[3], GLfloat facing, walkmap_ptr w)
{
    a->facing = facing;
    a->walkmap_data = walkmap_register_actor(w, a, pos, a->collision_radius);
    walkmap_set_movement_callback(w, a->walkmap_data, actor_movement_callback);

    // Update stored position
    walkmap_actor_position(w, a->walkmap_data, a->cached_position);
}

void actor_remove_from_walkmap(actor_ptr a, walkmap_ptr w)
{
    walkmap_unregister_actor(w, a->walkmap_data);
    a->walkmap_data = NULL;
}

/*
 * Render an actor into the current gl context
 *
 * Call Context: Main thread
 */
void actor_draw(actor_ptr a, modelview_ptr mv, renderer_ptr r)
{
    // Actor is not in the walkmap
    if (!a->walkmap_data)
        return;

    GLfloat mvp[16];
    GLfloat *modelview = modelview_push(mv);

    // Swap y and z axes for model
    // Move to new origin
    mtxTranslateApply(modelview, a->cached_position[0], a->cached_position[1], a->cached_position[2]);

    // Rotate facing
    mtxRotateZApply(modelview, a->facing);

    // Temporary: Translate test model to appear correctly
    mtxScaleApply(modelview, 0.1, 0.1, 0.1);
    mtxRotateXApply(modelview, 90);
    mtxTranslateApply(modelview, 0, 10, 0);
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_model_shader(r, mvp);

    model_draw(a->model, r);
    modelview_pop(mv);
}

void actor_velocity(actor_ptr a, GLfloat v[2], walkmap_ptr w)
{
    if (!a->walkmap_data)
    {
        printf("Warning: Attempting to access velocity of actor (%p) outside walkmap\n", a);
        v[0] = v[1] = 0;
        return;
    }

    walkmap_actor_velocity(w, a->walkmap_data, v);
}

void actor_set_velocity(actor_ptr a, GLfloat v[2], walkmap_ptr w)
{
    if (!a->walkmap_data)
    {
        printf("Warning: Attempting to set velocity of actor (%p) outside walkmap\n", a);
        return;
    }

    walkmap_set_actor_velocity(w, a->walkmap_data, v);
}

void actor_position(actor_ptr a, GLfloat p[3], walkmap_ptr w)
{
    if (!a->walkmap_data)
    {
        printf("Warning: Attempting to access position of actor (%p) outside walkmap\n", a);
        p[0] = p[1] = p[2] = 0;
        return;
    }

    walkmap_actor_position(w, a->walkmap_data, p);
}

void actor_set_position(actor_ptr a, GLfloat p[3], walkmap_ptr w)
{
    if (!a->walkmap_data)
    {
        printf("Warning: Attempting to access position of actor (%p) outside walkmap\n", a);
        return;
    }

    walkmap_set_actor_position(w, a->walkmap_data, p);
    walkmap_actor_position(w, a->walkmap_data, a->cached_position);
}

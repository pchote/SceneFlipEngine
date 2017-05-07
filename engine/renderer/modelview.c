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
#include "matrix.h"
#include "modelview.h"

/*
 * Private implementation detail
 */
struct modelview
{
    GLfloat *stack;
    size_t stack_size;
    size_t stack_max;

    GLfloat projection[16];
    GLfloat camera[16];

    // cached projection * camera
    GLfloat pc[16];
};

/*
 * Create a modelview object
 */
modelview_ptr modelview_create()
{
    modelview_ptr mv = calloc(1, sizeof(struct modelview));
    assert(mv);

    mv->stack_max = 10;
    mv->stack = calloc(16*mv->stack_max, sizeof(GLfloat));
    assert(mv->stack);

    mtxLoadIdentity(mv->camera);
    mtxLoadIdentity(mv->projection);
    mtxLoadIdentity(mv->pc);

    // Keep the identity at the bottom to simplify
    // calculating the mvp and pushing to an "empty" stack.
    mv->stack_size = 1;
    mtxLoadIdentity(mv->stack);
    return mv;
}

/*
 * Release resources associated with this actor
 */
void modelview_destroy(modelview_ptr mv)
{
    free(mv->stack);
    free(mv);
}

/*
 * Set a new projection matrix
 */
void modelview_set_projection(modelview_ptr mv, GLfloat p[16])
{
    memcpy(mv->projection, p, 16*sizeof(GLfloat));
    mtxMultiply(mv->pc, mv->projection, mv->camera);
}

/*
 * Set a new camera matrix 
 */
void modelview_set_camera(modelview_ptr mv, GLfloat c[16])
{
    memcpy(mv->camera, c, 16*sizeof(GLfloat));
    mtxMultiply(mv->pc, mv->projection, mv->camera);
}

/*
 * Add a new modelview step to the stack, returning
 * a pointer to be modified
 */
GLfloat *modelview_push(modelview_ptr mv)
{
    assert(mv->stack_size < mv->stack_max);
    GLfloat *last = &mv->stack[16*(mv->stack_size-1)];
    GLfloat *new = &mv->stack[16*mv->stack_size++];
    memcpy(new, last, 16*sizeof(GLfloat));
    return new;
}

/*
 * Discard the top modeview matrix
 */
void modelview_pop(modelview_ptr mv)
{
    assert(mv->stack_size > 1);
    mv->stack_size--;
}

/*
 * Calculate the final modelviewprojection matrix
 */
void modelview_calculate_mvp(modelview_ptr mv, GLfloat mvp[16])
{
    mtxMultiply(mvp, mv->pc, &mv->stack[16*(mv->stack_size-1)]);
}

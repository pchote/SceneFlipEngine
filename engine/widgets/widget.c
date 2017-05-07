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
#include "modelview.h"
#include "matrix.h"
#include "widget.h"
#include "widget_string.h"

/*
 * The Widget hierachy is a binary tree of siblings and children.
 * Each node contains an opaque implementation blob.
 */
struct widget
{
    char *id;
    enum widget_type type;
    void *data;
    GLfloat pos[2];

    widget_ptr child;
    widget_ptr next;
};

widget_ptr widget_create_root()
{
    widget_ptr w = calloc(1, sizeof(struct widget));
    assert(w);

    w->id = strdup("root");
    assert(w->id);

    w->type = WIDGET_CONTAINER;
    w->pos[0] = 0;
    w->pos[1] = 0;

    return w;
}

void widget_add(widget_ptr parent, const char *id, GLfloat pos[2], enum widget_type type, void *data)
{
    // Create a new child container
    widget_ptr w = calloc(1, sizeof(struct widget));
    assert(w);

    w->id = strdup(id);
    assert(w->id);

    w->type = type;
    w->data = data;
    memcpy(w->pos, pos, 2*sizeof(GLfloat));

    // Append to end of child list
    widget_ptr *slot = &parent->child;
    while (*slot)
        slot = &(*slot)->next;
    *slot = w;
}

void widget_destroy(widget_ptr w, engine_ptr e)
{
    free(w->id);

    switch (w->type)
    {
        case WIDGET_STRING: widget_string_destroy(w->data, e); break;
        case WIDGET_CONTAINER: default: break;
    }

    if (w->child)
        widget_destroy(w->child, e);
    if (w->next)
        widget_destroy(w->next, e);
}

void widget_draw(widget_ptr w, modelview_ptr mv, renderer_ptr r)
{
    GLfloat *modelview = modelview_push(mv);
    mtxTranslateApply(modelview, w->pos[0], w->pos[1], 0);

    // Draw self
    switch (w->type)
    {
        case WIDGET_STRING: widget_string_draw(w->data, mv, r); break;
        case WIDGET_CONTAINER: default: break;
    }

    // Draw children
    if (w->child)
        widget_draw(w->child, mv, r);

    modelview_pop(mv);

    // Draw sibling widget
    if (w->next)
        widget_draw(w->next, mv, r);
}

void widget_debug_draw(widget_ptr w, modelview_ptr mv, renderer_ptr r)
{
    GLfloat *modelview = modelview_push(mv);
    mtxTranslateApply(modelview, w->pos[0], w->pos[1], 0);

    // Draw self
    switch (w->type)
    {
        case WIDGET_STRING: widget_string_debug_draw(w->data, mv, r); break;
        case WIDGET_CONTAINER: default: break;
    }

    // Draw children
    if (w->child)
        widget_debug_draw(w->child, mv, r);

    modelview_pop(mv);

    // Draw sibling widget
    if (w->next)
        widget_debug_draw(w->next, mv, r);
}

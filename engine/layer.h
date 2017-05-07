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

#ifndef GamePrototype_layer_h
#define GamePrototype_layer_h

#include "typedefs.h"
#include "scene.h"

layer_ptr layer_create(const char *image, GLfloat *screen_region, GLfloat depth,
                       GLfloat *frame_regions, GLsizei frame_count, GLfloat *normal,
                       struct camera_state *camera, engine_ptr e);
void layer_destroy(layer_ptr l, engine_ptr e);
void layer_draw(layer_ptr l, modelview_ptr mv, renderer_ptr r);
void layer_debug_draw(layer_ptr l, modelview_ptr mv, renderer_ptr r);
GLfloat layer_render_order(layer_ptr l);

bool layer_visible(layer_ptr l);
void layer_set_visible(layer_ptr l, bool visible);
GLsizei layer_frame(layer_ptr l);
GLsizei layer_framecount(layer_ptr l);
void layer_set_frame(layer_ptr l, GLsizei i);

#endif

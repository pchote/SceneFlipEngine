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

#ifndef GamePrototype_model_h
#define GamePrototype_model_h

#include "typedefs.h"

model_ptr model_create(const char *path, engine_ptr e);
void model_destroy(model_ptr m, engine_ptr e);
void model_set_animation_frac(model_ptr m, GLfloat frac);
void model_draw(model_ptr m, renderer_ptr r);

void model_convert_obj(const char **input, size_t input_count, const char *output);
void model_set_animation_frac(model_ptr m, GLfloat frac);
void model_step_animation_frac(model_ptr m, GLfloat frac);

#endif

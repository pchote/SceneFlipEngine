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

/*
 * A layer is an image which is rendered within a scene
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include "engine.h"
#include "renderer.h"
#include "matrix.h"
#include "texture.h"
#include "vertexarray.h"
#include "modelview.h"
#include "scene.h"
#include "layer.h"

/*
 * Private implementation detail
 */
struct layer
{
    GLfloat y;

    texture_instance_ptr texture;
    vertexarray_ptr va;

    GLsizei frame_count;
    GLfloat *frame_regions;
    GLsizei frame;
    bool texcoords_dirty;

    bool visible;
};

/*
 * Calculate the 4 corner + center vectors defining the viewing fulstrum
 */
static void calculateViewFulstrum(GLfloat screen_region[4], struct camera_state *camera, GLfloat view[12])
{
    // Default viewing fulstrum:
    // - pointing along Y axis
    // - scaled so screen coords match y,z coords
    GLfloat y = 1/tan(camera->fov*M_PI/360);
    GLfloat fulstrum[15] =
    {
        screen_region[1], y, screen_region[3], // top right
        screen_region[0], y, screen_region[3], // top left
        screen_region[1], y, screen_region[2], // bottom right
        screen_region[0], y, screen_region[2], // bottom left

        // center
        (screen_region[0] + screen_region[1])/2, y, (screen_region[2] + screen_region[3])/2,
    };

    // Transform for rotating and scaling the viewing fulstrum
    // Note: left hand multiplication, so order of operations is reversed
    GLfloat transform[16];
    mtxLoadIdentity(transform);

    // Tilt plane so the normal points towards towards camera
    mtxRotateZApply(transform, -camera->yaw);
    mtxRotateXApply(transform, camera->pitch);

    // Scale plane based on distance to fill the entire viewing fulstrum
    mtxScaleApply(transform, 2, 1, 2/camera->aspect);

    // Transform from screen x,y coords to normalized device coords (origin at center)
    mtxTranslateApply(transform, -0.5, 0, -0.5);

    for (size_t i = 0; i < 5; i++)
        mtxMultiplyVec3(&view[3*i], transform, &fulstrum[3*i]);
}

/*
 * Create a layer
 * Layers are positioned with a given screen x,y,width,height
 * with a z-coordinate calculated from the given world y.
 *
 * Call Context: Worker thread
 */
layer_ptr layer_create(const char *image, GLfloat *screen_region, GLfloat depth,
                       GLfloat *frame_regions, GLsizei frame_count, GLfloat *normal,
                       struct camera_state *camera, engine_ptr e)
{
    layer_ptr l = calloc(1, sizeof(struct layer));
    if (!l)
        return NULL;

    l->visible = true;
    l->y = depth;

    // Calculate texture coords for each frame
    l->frame_count = frame_count;
    l->frame_regions = calloc(16*l->frame_count, sizeof(GLfloat));
    if (!frame_regions)
    {
        free(l);
        return NULL;
    }

    // Calculate view fulstrum
    GLfloat view[15];
    calculateViewFulstrum(screen_region, camera, view);

    // View center direction vector
    GLfloat *c = &view[12];

    // Distance along view vector to plane center
    GLfloat cd = (depth - camera->pos[1])/view[13];

    // Layer normal vector
    GLfloat n[3];

    if (normal)
        memcpy(n, normal, 3*sizeof(GLfloat));
    else
    {
        // Face towards the camera
        n[0] = -c[0];
        n[1] = -c[1];
        n[2] = -c[2];
    }

    // Ensure unit normal vector
    GLfloat nlen = sqrt(n[1]*n[1] + n[1]*n[1] + n[2]*n[2]);
    n[0] /= nlen;
    n[1] /= nlen;
    n[2] /= nlen;

    GLfloat *vertices = calloc(12, sizeof(GLfloat));
    assert(vertices);

    // Position layer vertices at the intersection of the
    // viewing fulstrum and the layer plane
    // Set texture q coordinate based on distance, so the
    // overall texture projection appears linear
    uint8_t xj[4] = {1,0,1,0};
    uint8_t yj[4] = {3,3,2,2};

    for (uint8_t j = 0; j < 4; j++)
    {
        // View corner direction vector
        GLfloat *vd = &view[3*j];

        GLfloat d = cd*(c[0]*n[0] + c[1]*n[1] + c[2]*n[2]) / (vd[0]*n[0] + vd[1]*n[1] + vd[2]*n[2]);
        vertices[3*j  ] = camera->pos[0] + d*vd[0];
        vertices[3*j+1] = camera->pos[1] + d*vd[1];
        vertices[3*j+2] = camera->pos[2] + d*vd[2];

        for (GLsizei i = 0; i < frame_count; i++)
        {
            l->frame_regions[16*i + 4*j] = frame_regions[4*i + xj[j]]*d;
            l->frame_regions[16*i + 4*j+1] = frame_regions[4*i + yj[j]]*d;
            l->frame_regions[16*i + 4*j+2] = 0;
            l->frame_regions[16*i + 4*j+3] = d;
        }
    }
    l->va = vertexarray_create(vertices, NULL, 4, 4, GL_TRIANGLE_STRIP, e);
    free(vertices);

    layer_set_frame(l, 0);
    l->texture = engine_retain_texture(e, image);

    return l;
}

/*
 * Destroy layer
 */
void layer_destroy(layer_ptr l, engine_ptr e)
{
    vertexarray_destroy(l->va, e);
    engine_release_texture(e, l->texture);
    free(l);
}

/*
 * Render layer into the current gl context
 *
 * Call Context: Main thread
 */
void layer_draw(layer_ptr l, modelview_ptr mv, renderer_ptr r)
{
    if (!l->visible)
        return;

    if (l->texcoords_dirty)
    {
        vertexarray_update(l->va, NULL, &l->frame_regions[16*l->frame], 4, GL_DYNAMIC_DRAW);
        l->texcoords_dirty = false;
    }

    GLfloat mvp[16];
    modelview_calculate_mvp(mv, mvp);
    renderer_enable_layer_shader(r, mvp);

    texture_bind(l->texture, GL_TEXTURE0);
    vertexarray_draw(l->va);
}

void layer_debug_draw(layer_ptr l, modelview_ptr mv, renderer_ptr r)
{
    if (!l->visible)
        return;

    GLfloat mvp[16];
    modelview_calculate_mvp(mv, mvp);

#if !PLATFORM_GLES
    GLfloat color[4] = {1,0,0,1};
    renderer_enable_line_shader(r, mvp, color);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    vertexarray_draw(l->va);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

/*
 * Get the world-y coordinate for determining render order
 */
GLfloat layer_render_order(layer_ptr l)
{
    return l->y;
}


bool layer_visible(layer_ptr l)
{
    return l->visible;
}

void layer_set_visible(layer_ptr l, bool visible)
{
    l->visible = visible;
}

GLsizei layer_frame(layer_ptr l)
{
    return l->frame;
}

GLsizei layer_framecount(layer_ptr l)
{
    return l->frame_count;
}

void layer_set_frame(layer_ptr l, GLsizei i)
{
    assert(i < l->frame_count);
    l->frame = i;
    l->texcoords_dirty = true;
}

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
#include <png.h>

#include "renderer.h"
#include "texture.h"
#include "engine.h"

struct texture
{
    char *path;
    GLuint glid;

    // Initialization info
    png_uint_32 width;
    png_uint_32 height;
    png_byte *image_data;
    bool initialized;
};

/*
 * Initialize the texture gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
void init_gl(void *_t)
{
    texture_ptr t = _t;
    if (t->initialized)
    {
        // May be called by draw before engine runs the task
        printf("Attempting to initialize already initialized texture.\n");
        return;
    }

    // Now generate the OpenGL texture object
    glGenTextures(1, &t->glid); checkGLError();
    glActiveTexture(GL_TEXTURE0); checkGLError();
    glBindTexture(GL_TEXTURE_2D, t->glid); checkGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid *)t->image_data); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); checkGLError();
	glGenerateMipmap(GL_TEXTURE_2D); checkGLError();

    free(t->image_data);
    t->image_data = NULL;
    t->initialized = true;
}

/*
 * Uninitialize the texture gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void uninit_gl(void *_t)
{
    texture_ptr t = _t;
    assert(t->initialized);

    glDeleteTextures(1, &t->glid);
    free(t->path);
    free(t->image_data);
    free(t);
}

/*
 * Create a texture object
 */
texture_ptr texture_create(const char *path, engine_ptr e)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return NULL;

    // Test that this is actually a png
    png_byte header[8];
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
    {
        fclose(fp);
        return NULL;
    }

    // Initialize metadata storage
    png_structp png_t = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_t)
    {
        fclose(fp);
        return NULL;
    }

    png_infop info_t = png_create_info_struct(png_t);
    png_infop end_info = png_create_info_struct(png_t);
    if (!info_t || !end_info)
    {
        png_destroy_read_struct(&png_t, &info_t, &end_info);
        fclose(fp);
        return NULL;
    }

    texture_ptr t = calloc(1, sizeof(struct texture));
    assert(t);
    t->path = strdup(path);

    // Set libpng jumpbuf for catching internal errors
    if (!setjmp(png_jmpbuf(png_t)))
    {
        int bit_depth, color_type;

        // Skip fileheader
        png_init_io(png_t, fp);
        png_set_sig_bytes(png_t, 8);

        // Read metadata
        png_read_info(png_t, info_t);
        png_get_IHDR(png_t, info_t, &t->width, &t->height, &bit_depth, &color_type, NULL, NULL, NULL);
        png_read_update_info(png_t, info_t);
        png_size_t rowbytes = png_get_rowbytes(png_t, info_t);

        // iOS requires square textures
        assert(t->width == t->height);

        t->image_data = calloc(rowbytes*t->height, sizeof(png_byte));
        assert(t->image_data);

        // Prepare points to each row for loading image data
        png_bytep *row_pointers = calloc(t->height, sizeof(png_bytep));
        assert(row_pointers);
        for (int i = 0; i < t->height; ++i)
            row_pointers[t->height - 1 - i] = t->image_data + i * rowbytes;
        png_read_image(png_t, row_pointers);

        // Cleanup
        free(row_pointers);
    }
    else
    {
        // png error
        if (t->image_data)
            free(t->image_data);
        free(t);
        t = NULL;
    }

    png_destroy_read_struct(&png_t, &info_t, &end_info);
    fclose(fp);

    engine_queue_task(e, init_gl, t);
    return t;
}

/*
 * Destroy texture
 *
 * Call Context: Worker thread
 */
void texture_destroy(texture_ptr t, engine_ptr e)
{
    engine_queue_task(e, uninit_gl, t);
}

/*
 * Destroy texture from an engine task
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
void texture_destroy_internal(texture_ptr t)
{
    uninit_gl(t);
}

/*
 * Bind a texture to the request GL attachment point
 *
 * Call Context: Main thread
 */
void texture_bind(texture_instance_ptr t, GLenum attachment)
{
    if (!t->initialized)
    {
        printf("WARNING: Attempting to access uninitialized texture. Initializing on hot path.\n");
        init_gl((void *)t);
    }

    glActiveTexture(attachment);
    glBindTexture(GL_TEXTURE_2D, t->glid);
}

bool texture_has_path(texture_ptr t, const char *path)
{
    return strcmp(path, t->path) == 0;
}


// TODO: This is shit
textureref texture_get_textureref(texture_ptr t, GLfloat width, GLfloat height)
{
    return (textureref)
    {
        .texture = t->glid,
        .width = width,
        .height = height
    };
}

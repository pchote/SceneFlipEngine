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

#include <png.h>
#include <assert.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "font.h"
#include "engine.h"
#include "renderer.h"

struct font_glyph
{
    GLfloat advance;
    GLfloat offset[2];
    GLfloat pos[2];
    GLfloat size[2];
};

struct font
{
    GLuint glid;
    bool initialized;
    struct font_glyph glyphs[96];
    GLfloat line_height;
    GLfloat scale;

    // Texture info, used only during initialization
    GLsizei size;
    uint8_t *data;
};

/*
 * Initialize the font gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void init_gl(void *_f)
{
    font_ptr f = _f;

    if (f->initialized)
    {
        // May be called by draw before engine runs the task
        printf("Attempting to initialize already initialized font.\n");
        return;
    }

    // Generate texture
    glGenTextures(1, &f->glid);
    glActiveTexture(GL_TEXTURE0); checkGLError();
    glBindTexture(GL_TEXTURE_2D, f->glid); checkGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, f->size, f->size, 0, 6403, GL_UNSIGNED_BYTE, (GLvoid *)f->data); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); checkGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); checkGLError();

    f->initialized = true;
}

/*
 * Uninitialize the font gl state
 *
 * Call Context: Main thread (via engine_process_tasks)
 */
static void uninit_gl(void *_f)
{
    font_ptr f = _f;
    assert(f->initialized);

    glDeleteTextures(1, &f->glid);
    free(f);
}

struct font_glyph *glyph_ptr(font_ptr f, uint32_t c)
{
    // Return <space> for unknown glyphs
    GLsizei index = 0;
    if (c >= 0x20 && c < 0x7f)
        index = c - 0x20;

    return &f->glyphs[index];
}

font_ptr font_create(const char *path, GLuint font_size, GLuint texture_size, GLfloat scale, engine_ptr e)
{
    font_ptr f = calloc(1, sizeof(struct font));
    assert(f);

    FT_Library ft;
    FT_Face face;

    if (FT_Init_FreeType(&ft))
    {
        printf("Unable to initialize freetype\n");
        assert(FATAL_ERROR);
    }
    
    if (FT_New_Face(ft, path, 0, &face))
    {
        printf("Unable to load font %s\n", path);
        assert(FATAL_ERROR);
    }

    if (FT_Set_Pixel_Sizes(face, 0, font_size))
    {
        printf("Unable to set %dpx size for %s\n", font_size, path);
        assert(FATAL_ERROR);
    }

    f->size = texture_size;
    f->scale = scale;
    f->data = calloc(f->size*f->size, sizeof(uint8_t));
    assert(f->data);

    // Cache printable ascii characters
    GLsizei cur_height = 0;
    GLsizei cur_x = 0;
    GLsizei cur_y = 0;

    for (uint32_t c = 0x20; c < 0x7f; c++)
    {
        FT_GlyphSlot gs = face->glyph;
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            printf("Error loading glyph for charcode %d\n", c);
            assert(FATAL_ERROR);
        }

        // Start a new row
        if (cur_x + gs->bitmap.width >= f->size)
        {
            cur_x = 0;
            cur_y += cur_height + 1;
            cur_height = 0;
        }

        // Check for texture overflow
        if (cur_y + gs->bitmap.rows >= f->size)
        {
            printf("Font texture overflow on charcode %x (%c)\n", c, c);
            assert(FATAL_ERROR);
        }

        // Copy pixel data
        uint8_t *gb = gs->bitmap.buffer;
        for (GLsizei y = 0; y < gs->bitmap.rows; y++)
            for (GLsizei x = 0; x < gs->bitmap.width; x++)
                f->data[f->size*(cur_y+y) + cur_x + x] = *gb++;

        // Store glyph info
        GLfloat s = 1.0f / f->size;
        struct font_glyph *fg = glyph_ptr(f, c);
        fg->pos[0] = s*cur_x;
        fg->pos[1] = s*cur_y;
        fg->size[0] = s*gs->bitmap.width;
        fg->size[1] = s*gs->bitmap.rows;
        fg->offset[0] = s*gs->metrics.horiBearingX / 64.0f;
        fg->offset[1] = s*(gs->metrics.horiBearingY - gs->metrics.height) / 64.0f;
        fg->advance = s*gs->metrics.horiAdvance / 64.0f;

        // Increment texture position
        cur_x += gs->bitmap.width + 1;
        if (cur_height < gs->bitmap.rows)
            cur_height = gs->bitmap.rows;
    }

    f->line_height = face->size->metrics.height / 64.0f / f->size;

    engine_queue_task(e, init_gl, f);
    return f;
}

/*
 * Destroy font
 */
void font_destroy(font_ptr f, engine_ptr e)
{
    engine_queue_task(e, uninit_gl, f);
}

void font_bind_texture(font_instance_ptr f)
{
    if (!f->initialized)
    {
        printf("WARNING: Attempting to access uninitialized font. Initializing on hot path.\n");
        init_gl((void *)f);
    }

    glBindTexture(GL_TEXTURE_2D, f->glid); checkGLError();
}

struct parser_state
{
    const char *str;
    GLfloat pos[2];
    GLfloat color[4];
};

static bool parse_formatting_command(font_instance_ptr f, struct parser_state *s, uint32_t type, char *arg)
{
    switch (type)
    {
        case 'c':
            if (arg[0] != '#' || strlen(arg) != 9)
            {
                printf("Color argument must be a RGBA hex code.\n");
                return false;
            }

            // Extract color code
            uint32_t color;
            sscanf(arg, "#%x", &color);
            for (uint8_t i = 0; i < 4; i++)
                s->color[i] = (color >> ((3-i)*8) & 0xFF) / 255.0f;

            return true;
        break;
        default:
            printf("Unknown formatting type '%c'\n", type);
        return false;
    }
    return true;
}

/*
 * Return the next printable character in a string, parsing any
 * embedded formatting information
 */
static uint32_t parse_next_character(font_instance_ptr f, struct parser_state *s)
{
    uint32_t c = '\0';

    const uint8_t max_arg_length = 10;
    char arg_buf[max_arg_length+1];
    while ((c = *s->str++) != '\0')
    {
        if (c == '\n')
        {
            s->pos[0] = 0;
            s->pos[1] -= f->scale*f->line_height;
            continue;
        }

        // Formatting commands start with an unescaped '\'
        if (c == '\\')
        {
            // Type code
            uint32_t t = *s->str++;

            // Literal '\'
            if (t == '\\')
                return t;

            // Check for argument string
            if (*s->str++ != '[')
            {
                printf("Invalid formatting character %c: Missing argument string.\n", t);
                // Rewind and display as regular text
                s->str -= 2;
                return *s->str++;
            }

            // Read argument string
            uint8_t i;
            for (i = 0; i < max_arg_length; i++)
            {
                arg_buf[i] = *s->str++;
                if (arg_buf[i] == ']')
                {
                    arg_buf[i] = '\0';
                    if (parse_formatting_command(f, s, t, arg_buf))
                        break;
                    else
                    {
                        // Error parsing command
                        s->str -= 4 + i;
                        return *s->str++;
                    }
                }
            }

            // Check for argument overflow
            if (i == max_arg_length)
            {
                printf("Invalid formatting character %c: Argument string overflow.\n", t);
                // Rewind and display as regular text
                s->str -= 3 + max_arg_length;
                return *s->str++;
            }
            continue;
        }

        // Glyphs with no renderable component (eg ' ')
        struct font_glyph *fg = glyph_ptr((font_ptr)f, c);
        if (fg->size[0] == 0 || fg->size[1] == 0)
        {
            s->pos[0] += f->scale*fg->advance;
            continue;
        }

        break;
    }
    return c;
}

/*
 * Return the number of glyphs that will be drawn for a given string
 */
GLsizei font_string_glyph_count(font_instance_ptr f, char *str)
{
    struct parser_state s;
    s.str = str;
    GLsizei size = 0;
    while (parse_next_character(f, &s) != '\0')
        size++;

    return size;
}

void font_render_string(font_instance_ptr f, const char *str, GLsizei len, GLfloat *buffer)
{
    struct parser_state s;
    s.pos[0] = 0;
    s.pos[1] = 0;
    s.color[0] = 1;
    s.color[1] = 1;
    s.color[2] = 1;
    s.color[3] = 1;
    s.str = str;

    uint8_t xj[6] = {1,1,0,0,0,1};
    uint8_t yj[6] = {2,3,3,3,2,2};

    GLsizei size = 0;
    while (size < len)
    {
        uint32_t c = parse_next_character(f, &s);
        if (c == '\0')
            break;

        struct font_glyph *fg = glyph_ptr((font_ptr)f, c);
        GLfloat vr[4] = {fg->offset[0], fg->offset[0] + fg->size[0],
                         fg->offset[1] + fg->size[1], fg->offset[1]};
        GLfloat tr[4] = {fg->pos[0], fg->pos[0] + fg->size[0],
                         fg->pos[1], fg->pos[1] + fg->size[1]};
        for (uint8_t j = 0; j < 6; j++)
        {
            *buffer++ = s.pos[0] + f->scale*vr[xj[j]];
            *buffer++ = s.pos[1] + f->scale*vr[yj[j]];
            *buffer++ = 0;
            *buffer++ = tr[xj[j]];
            *buffer++ = tr[yj[j]];
            for (uint8_t k = 0; k < 4; k++)
                *buffer++ = s.color[k];
        }

        s.pos[0] += f->scale*fg->advance;
        size++;
    }
}

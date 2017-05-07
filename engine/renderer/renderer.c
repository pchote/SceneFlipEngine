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

#include "renderer.h"

/*
 * Private implementation details
 */

struct renderer
{
    GLuint layer_shader;
    GLuint layer_mvp_matrix_uniform;

    GLuint model_shader;
    GLuint model_mvp_matrix_uniform;

    GLuint text_shader;
    GLuint text_mvp_matrix_uniform;

    GLuint line_shader;
    GLuint line_mvp_matrix_uniform;
    GLuint line_color_uniform;

    GLuint line_color_shader;
    GLuint line_color_mvp_matrix_uniform;

    GLuint transition_shader;
    GLuint transition_mvp_matrix_uniform;
    GLuint transition_dt_uniform;
};


#pragma mark Shader Functions

/*
 * Load a shader source from file, prepend a version string,
 * and return a char buffer ready to compile
 */
static char *load_shader_source(const char *path)
{
    // Load shader source and check length
    FILE *shader_file = fopen(path, "r");
    if (!shader_file)
        return NULL;

    fseek(shader_file, 0, SEEK_END);
    size_t shader_size = ftell(shader_file);
    rewind(shader_file);

    // Prepend the version string at runtime to allow cross-platform support
    float glLanguageVersion;
#if PLATFORM_GLES
	sscanf((char *)glGetString(GL_SHADING_LANGUAGE_VERSION), "OpenGL ES GLSL ES %f", &glLanguageVersion);
#else
	sscanf((char *)glGetString(GL_SHADING_LANGUAGE_VERSION), "%f", &glLanguageVersion);
#endif
    GLuint version = 100 * glLanguageVersion;

    // Load the shader into a char buffer
    // Allocate enough space to prepend a 3 digit version string
    size_t total_size = shader_size + 14*sizeof(char);
    GLchar *shader_source = malloc(total_size);
    assert(shader_source);
	size_t offset = sprintf(shader_source, "#version %03d\n", version);
    fread(shader_source+offset, shader_size, 1, shader_file);
	fclose(shader_file);
    shader_source[total_size - 1] = '\0';
    return shader_source;
}

/*
 * Compile a shader ready for linking to a program
 */
static GLint compile_shader(GLenum type, const char *path)
{
    GLint shader = glCreateShader(type); checkGLError();
    const GLchar *shader_source = load_shader_source(path);
    assert(shader_source);

    glShaderSource(shader, 1, &shader_source, NULL); checkGLError();
    glCompileShader(shader); checkGLError();

    GLint logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    checkGLError();
    if (logLength > 0)
    {
        GLchar *log = malloc((size_t)logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, log); checkGLError();

        fprintf(stderr, "Shader compilation failed with error:\n%s", log);
        free(log);
        fprintf(stderr, "%s", shader_source);
        assert(FATAL_ERROR);
    }
    free((void *)shader_source);

    return shader;
}

/*
 * Attach shaders to a program and compile/link it ready to use
 */
static GLuint shader_init(const char *vertex_path, const char *fragment_source,
                          void (*bind_attributes_func)(GLuint))
{
    GLuint shader = glCreateProgram(); checkGLError();
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_path);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    (*bind_attributes_func)(shader);

    glAttachShader(shader, vs); checkGLError();
    glAttachShader(shader, fs); checkGLError();
    glLinkProgram(shader); checkGLError();

    GLint logLength;
    glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &logLength); checkGLError();
    if (logLength > 0)
    {
        GLchar *log = malloc((size_t)logLength);
        glGetProgramInfoLog(shader, logLength, &logLength, log); checkGLError();
        fprintf(stderr,"Shader program linking failed with error:\n%s", log);
        free(log);
    }

    return shader;
}

/*
 * Cleanup any resources associated with a shader program
 */
void shader_destroy(GLuint shader)
{
	if(!shader)
		return;

    // Find the shaders attached to the program
	GLsizei shaderCount;
	glGetProgramiv(shader, GL_ATTACHED_SHADERS, &shaderCount);
	GLuint* shaders = malloc(shaderCount * sizeof(GLuint));
    assert(shaders);

	// Get the names of the shaders attached to the program
	glGetAttachedShaders(shader, shaderCount, &shaderCount, shaders);

	// Delete the shaders attached to the program
	for(GLsizei i = 0; i < shaderCount; i++)
		glDeleteShader(shaders[i]);

	free(shaders);

	// Delete the program
	glDeleteProgram(shader);
	glUseProgram(0);
}

static void bind_layer_attributes(GLuint shader)
{
    glBindAttribLocation(shader, VERTEX_POS_ATTRIB_IDX, "aVertexPosition");
    glBindAttribLocation(shader, TEXTURE_COORDS_ATTRIB_IDX, "aVertexTexcoord");
}

static void init_layer_shader(renderer_ptr r)
{
    r->layer_shader = shader_init("shaders/layer.vsh", "shaders/layer.fsh", bind_layer_attributes);
    r->layer_mvp_matrix_uniform = glGetUniformLocation(r->layer_shader, "modelViewProjectionMatrix"); checkGLError();

    // Bind texture unit 0 to textureSampler then forget about it
    GLuint ts_uniform = glGetUniformLocation(r->layer_shader, "textureSampler"); checkGLError();
    glUseProgram(r->layer_shader);
    glUniform1i(ts_uniform, 0); checkGLError();
}

static void bind_model_attributes(GLuint shader)
{
    glBindAttribLocation(shader, VERTEX_POS_ATTRIB_IDX, "aVertexPosition");
    glBindAttribLocation(shader, TEXTURE_COORDS_ATTRIB_IDX, "aVertexTexcoord");
}

static void init_model_shader(renderer_ptr r)
{
    r->model_shader = shader_init("shaders/model.vsh", "shaders/model.fsh", bind_model_attributes);
    r->model_mvp_matrix_uniform = glGetUniformLocation(r->model_shader, "modelViewProjectionMatrix"); checkGLError();

    // Bind texture unit 0 to textureSampler then forget about it
    GLuint ts_uniform = glGetUniformLocation(r->model_shader, "textureSampler"); checkGLError();
    glUseProgram(r->model_shader);
    glUniform1i(ts_uniform, 0); checkGLError();
}

static void bind_text_attributes(GLuint shader)
{
    glBindAttribLocation(shader, VERTEX_POS_ATTRIB_IDX, "aVertexPosition");
    glBindAttribLocation(shader, TEXTURE_COORDS_ATTRIB_IDX, "aVertexTexcoord");
    glBindAttribLocation(shader, COLOR_ATTRIB_IDX, "aVertexColor");
}

static void init_text_shader(renderer_ptr r)
{
    r->text_shader = shader_init("shaders/text.vsh", "shaders/text.fsh", bind_text_attributes);
    r->text_mvp_matrix_uniform = glGetUniformLocation(r->text_shader, "modelViewProjectionMatrix"); checkGLError();

    // Bind texture unit 0 to textureSampler then forget about it
    GLuint ts_uniform = glGetUniformLocation(r->text_shader, "textureSampler"); checkGLError();
    glUseProgram(r->text_shader);
    glUniform1i(ts_uniform, 0); checkGLError();
}

static void bind_line_attributes(GLuint shader)
{
    glBindAttribLocation(shader, VERTEX_POS_ATTRIB_IDX, "aVertexPosition");
}

static void init_line_shader(renderer_ptr r)
{
    r->line_shader = shader_init("shaders/line.vsh", "shaders/line.fsh", bind_line_attributes);
    r->line_mvp_matrix_uniform = glGetUniformLocation(r->line_shader, "modelViewProjectionMatrix"); checkGLError();
    r->line_color_uniform = glGetUniformLocation(r->line_shader, "color"); checkGLError();
}


static void bind_line_color_attributes(GLuint shader)
{
    glBindAttribLocation(shader, VERTEX_POS_ATTRIB_IDX, "aVertexPosition");
    glBindAttribLocation(shader, COLOR_ATTRIB_IDX, "aVertexColor");
}

static void init_line_color_shader(renderer_ptr r)
{
    r->line_color_shader = shader_init("shaders/line-color.vsh", "shaders/line-color.fsh", bind_line_color_attributes);
    r->line_color_mvp_matrix_uniform = glGetUniformLocation(r->line_color_shader, "modelViewProjectionMatrix"); checkGLError();
}


static void bind_transition_attributes(GLuint shader)
{
    glBindAttribLocation(shader, VERTEX_POS_ATTRIB_IDX, "aVertexPosition");
    glBindAttribLocation(shader, TEXTURE_COORDS_ATTRIB_IDX, "aVertexTexcoord");
}

static void init_transition_shader(renderer_ptr r)
{
    r->transition_shader = shader_init("shaders/transition.vsh", "shaders/transition.fsh", bind_transition_attributes);
    r->transition_mvp_matrix_uniform = glGetUniformLocation(r->transition_shader, "modelViewProjectionMatrix"); checkGLError();
    r->transition_dt_uniform = glGetUniformLocation(r->transition_shader, "dt"); checkGLError();

    // Bind texture unit 0 to textureSampler then forget about it
    GLuint ts_uniform = glGetUniformLocation(r->transition_shader, "textureSampler"); checkGLError();
    GLuint ts2_uniform = glGetUniformLocation(r->transition_shader, "textureSampler2"); checkGLError();
    glUseProgram(r->transition_shader);
    glUniform1i(ts_uniform, 0); checkGLError();
    glUniform1i(ts2_uniform, 1); checkGLError();
}

#pragma mark Public functions
renderer_ptr renderer_create()
{
    renderer_ptr r = calloc(1, sizeof(struct renderer));
    assert(r);

    init_layer_shader(r);
    init_model_shader(r);
    init_text_shader(r);
    init_line_shader(r);
    init_line_color_shader(r);
    init_transition_shader(r);

    return r;
}

void renderer_destroy(renderer_ptr r)
{
    shader_destroy(r->model_shader);
    shader_destroy(r->line_shader);
}

/*
 * Enable the requested shader and set the mvp shader param
 */
void renderer_enable_layer_shader(renderer_ptr r, GLfloat mvp[16])
{
    glUseProgram(r->layer_shader); checkGLError();
    glUniformMatrix4fv(r->layer_mvp_matrix_uniform, 1, GL_FALSE, mvp); checkGLError();
}

void renderer_enable_model_shader(renderer_ptr r, GLfloat mvp[16])
{
    glUseProgram(r->model_shader); checkGLError();
    glUniformMatrix4fv(r->model_mvp_matrix_uniform, 1, GL_FALSE, mvp); checkGLError();
}

void renderer_enable_text_shader(renderer_ptr r, GLfloat mvp[16])
{
    glUseProgram(r->text_shader); checkGLError();
    glUniformMatrix4fv(r->text_mvp_matrix_uniform, 1, GL_FALSE, mvp); checkGLError();
}

void renderer_enable_line_shader(renderer_ptr r, GLfloat mvp[16], GLfloat color[4])
{
    glUseProgram(r->line_shader); checkGLError();
    glUniformMatrix4fv(r->line_mvp_matrix_uniform, 1, GL_FALSE, mvp); checkGLError();
    glUniform4fv(r->line_color_uniform, 1, color); checkGLError();
}

void renderer_enable_line_color_shader(renderer_ptr r, GLfloat mvp[16])
{
    glUseProgram(r->line_color_shader); checkGLError();
    glUniformMatrix4fv(r->line_color_mvp_matrix_uniform, 1, GL_FALSE, mvp); checkGLError();
}

void renderer_enable_transition_shader(renderer_ptr r, GLfloat mvp[16], double dt)
{
    glUseProgram(r->transition_shader); checkGLError();
    glUniformMatrix4fv(r->transition_mvp_matrix_uniform, 1, GL_FALSE, mvp); checkGLError();
    glUniform1f(r->transition_dt_uniform, dt); checkGLError();
}

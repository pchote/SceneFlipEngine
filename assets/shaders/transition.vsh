
#ifdef GL_ES
precision highp float;
#endif

#if __VERSION__ >= 140
in vec3 aVertexPosition;
in vec2 aVertexTexcoord;
out vec2 vTexcoord;
#else
attribute vec3 aVertexPosition;
attribute vec2 aVertexTexcoord;
varying vec2 vTexcoord;
#endif
uniform mat4 modelViewProjectionMatrix;

void main (void)
{
    vTexcoord = aVertexTexcoord;
    gl_Position = modelViewProjectionMatrix*vec4(aVertexPosition, 1.0);
}
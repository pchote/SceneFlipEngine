
#ifdef GL_ES
precision highp float;
#endif

#if __VERSION__ >= 140
in vec3 aVertexPosition;
in vec2 aVertexTexcoord;
in vec4 aVertexColor;
out vec2 vTexcoord;
out vec4 vColor;
#else
attribute vec3 aVertexPosition;
attribute vec2 aVertexTexcoord;
attribute vec4 aVertexColor;
varying vec2 vTexcoord;
varying vec4 vColor;
#endif
uniform mat4 modelViewProjectionMatrix;

void main (void)
{
    vTexcoord = aVertexTexcoord;
    vColor = aVertexColor;
    gl_Position = modelViewProjectionMatrix*vec4(aVertexPosition, 1.0);
}
#if __VERSION__ >= 140
in vec3 aVertexPosition;
#else
attribute vec3 aVertexPosition;
#endif
uniform mat4 modelViewProjectionMatrix;

void main (void)
{
    gl_Position = modelViewProjectionMatrix*vec4(aVertexPosition, 1.0);
}
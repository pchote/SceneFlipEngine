
#ifdef GL_ES
precision highp float;
#endif

#if __VERSION__ >= 140
out vec4 fragColor;
#endif

uniform vec4 color;

void main(void)
{
#if __VERSION__ >= 140
    fragColor = color;
#else
    gl_FragColor = color;
#endif
}
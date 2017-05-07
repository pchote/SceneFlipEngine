
#ifdef GL_ES
precision highp float;
#endif

#if __VERSION__ >= 140
in vec4 vColor;
out vec4 fragColor;
#else
varying vec4 vColor;
#endif

void main(void)
{
#if __VERSION__ >= 140
    fragColor = vColor;
#else
    gl_FragColor = vColor;
#endif
}
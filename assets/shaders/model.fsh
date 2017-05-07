
#ifdef GL_ES
precision highp float;
#endif

#if __VERSION__ >= 140
in vec2 vTexcoord;
out vec4 fragColor;
#else
varying vec2 vTexcoord;
#endif

uniform sampler2D textureSampler;

void main(void)
{
#if __VERSION__ >= 140
    fragColor = texture(textureSampler, vTexcoord.st, 0.0);
#else
    gl_FragColor = texture2D(textureSampler, vTexcoord.st, 0.0);
#endif
}
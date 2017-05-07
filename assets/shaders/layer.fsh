
#ifdef GL_ES
precision highp float;
#endif

#if __VERSION__ >= 140
in vec4 vTexcoord;
out vec4 fragColor;
#else
varying vec4 vTexcoord;
#endif

uniform sampler2D textureSampler;

void main(void)
{
#if __VERSION__ >= 140
    fragColor = textureProj(textureSampler, vTexcoord, 0.0);
#else
    gl_FragColor = texture2DProj(textureSampler, vTexcoord, 0.0);
#endif
}

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
uniform sampler2D textureSampler2;
uniform float dt;

void main(void)
{
#if __VERSION__ >= 140
    fragColor = dt*texture(textureSampler, vTexcoord.st, 0.0) + (1.0 - dt)*texture(textureSampler2, vTexcoord.st, 0.0);
#else
    gl_FragColor = dt*texture2D(textureSampler, vTexcoord.st, 0.0) + (1.0 - dt)*texture2D(textureSampler2, vTexcoord.st, 0.0);
#endif
}
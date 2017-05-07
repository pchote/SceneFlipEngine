
#ifdef GL_ES
precision highp float;
#endif

#if __VERSION__ >= 140
in vec2 vTexcoord;
in vec4 vColor;
out vec4 fragColor;
#else
varying vec2 vTexcoord;
varying vec4 vColor;
#endif

uniform sampler2D textureSampler;

void main(void)
{
    vec4 mask = vec4(1,0,0,0);
    vec4 c = vColor;
#if __VERSION__ >= 140
    c.a *= dot(texture(textureSampler, vTexcoord.st, 0.0), mask);
    fragColor = c;
#else
    c.a *= dot(texture2D(textureSampler, vTexcoord.st, 0.0), mask);
    gl_FragColor = c;
#endif
}
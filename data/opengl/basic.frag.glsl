#version 300 es
precision highp float;

in vec2 fTexCoord;

out vec4 fFragColor;

uniform sampler2D tDiffuse;

void main()
{
    fFragColor = texture(tDiffuse, fTexCoord);
    // oFragColor = vec4(1.0f, 1.0f, 0.0f, 1.0f);
} 
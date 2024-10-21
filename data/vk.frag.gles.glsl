#version 300 es
precision mediump float;
precision highp int;

struct PushConstants
{
    highp mat4 model;
    highp mat4 view;
    highp vec3 matColor;
};

uniform PushConstants _14;

uniform highp sampler2D albedo;

layout(location = 0) out highp vec4 color;
in highp vec2 uv;
in highp vec3 pos;
in highp vec3 norm;

void main()
{
    color = vec4(_14.matColor, 1.0) * texture(albedo, uv);
}


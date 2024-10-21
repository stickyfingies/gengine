#version 300 es

layout(std140) uniform UniformBufferObject
{
    mat4 proj;
} _69;

struct PushConstants
{
    mat4 model;
    mat4 view;
    vec3 matColor;
};

uniform PushConstants _14;

out vec3 pos;
layout(location = 0) in vec3 aPos;
out vec3 norm;
layout(location = 1) in vec3 aNorm;
out vec2 uv;
layout(location = 2) in vec2 aUv;

void main()
{
    pos = vec3((_14.model * vec4(aPos.x, -aPos.y, aPos.z, 1.0)).xyz);
    mat4 _45 = transpose(inverse(_14.model));
    norm = mat3(_45[0].xyz, _45[1].xyz, _45[2].xyz) * aNorm;
    uv = aUv;
    gl_Position = ((_69.proj * _14.view) * _14.model) * vec4(aPos.x, -aPos.y, aPos.z, 1.0);
}


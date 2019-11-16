#version 450 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;

layout (location = 0) out vec4 color;

const vec3 light_pos = {-10.0, 30.0, 10.0};

void main()
{
	const vec3 light_dir = normalize(light_pos - pos);

	color = max(dot(norm, light_dir), 0.0) * vec4(1.0, 0.0, 1.0, 1.0);
}
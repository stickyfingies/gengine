#version 450 core

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 norm;
layout (location = 2) in vec2 uv;

layout (location = 0) out vec4 color;

layout (binding = 0) uniform sampler2D albedo;

const vec3 light_pos = {-10.0, 30.0, 10.0};

void main()
{
	// const vec3 light_dir = normalize(light_pos - pos); // point light
	const vec3 light_dir = normalize(vec3(1.0, 1.0, 1.0)); // directional light

	color = 1.0 * texture(albedo, uv);
}

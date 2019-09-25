#version 330 core

layout (location = 0) in vec3 aPos;

layout (push_constant) uniform PushConstants
{
	mat4 model;
	mat4 view;
	mat4 projection;
};

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
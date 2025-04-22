-- RUN: cmake --workflow --preset linux-vk-dev && ./artifacts/linux-vk-dev/gpu/gpu-lua
local vertices =
    {-0.5, -0.5, 1.0, 0.0, 0.0, 0.5, -0.5, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 0.0, 1.0, -0.5, 0.5, 1.0, 1.0, 1.0}

local indices = {0, 1, 2, 2, 3, 0}

local vertex_shader_source = [[
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout (push_constant) uniform PushConstants
{
	mat4 model;
	mat4 view;
	vec3 matColor;
} pc;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
]]

local fragment_shader_source = [[
#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
]]

-- sho contains the target shaders and metadata about their structure
local sho = compile_shaders(vertex_shader_source, fragment_shader_source);

local pso = gpu:create_pipeline(sho.target_vertex_shader, sho.target_fragment_shader, sho.vertex_attributes, WindingOrder.CLOCKWISE)

local vbo = gpu:create_buffer(BufferUsage.VERTEX, 20, 4, vertices)
local ebo = gpu:create_buffer(BufferUsage.INDEX, 4, #indices, indices)

local geo = gpu:create_geometry(pso, vbo, ebo)

print("Hello, Window!")
while not shouldClose() do
    pollEvents()
    gpu:simple_draw(pso, geo);
    swapBuffers()
end
print("Goodbye, Window!");

gpu:destroy_buffer(ebo)
gpu:destroy_buffer(vbo)
gpu:destroy_pipeline(pso)

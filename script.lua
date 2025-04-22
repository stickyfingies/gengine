-- RUN: cmake --workflow --preset linux-vk-dev && ./artifacts/linux-vk-dev/gpu/gpu-lua
local vertices =
    {-0.5, -0.5, 1.0, 0.0, 0.0, 0.5, -0.5, 0.0, 1.0, 0.0, 0.5, 0.5, 0.0, 0.0, 1.0, -0.5, 0.5, 1.0, 1.0, 1.0}

local indices = {0, 1, 2, 2, 3, 0}

local vertex_shader_source = [[
#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

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

-- local vscglsl = vertex_shader_source
-- local vscsprv = glsl_to_sprv("v", ShaderStage.VERTEX, vscglsl, false)

-- local fscglsl = fragment_shader_source
-- local fscsprv = glsl_to_sprv("f", ShaderStage.FRAGMENT, fscglsl, false)

local vsc = compile_shader(ShaderStage.VERTEX, vertex_shader_source)
local fsc = compile_shader(ShaderStage.FRAGMENT, fragment_shader_source)

-- # for vulkan
local pso = gpu:create_pipeline(vsc, fsc, {VertexAttribute.VEC2, VertexAttribute.VEC3}, WindingOrder.CLOCKWISE)

-- # for opengl
-- local vscgles = sprv_to_gles(vscsprv)
-- local fscgles = sprv_to_gles(fscsprv)
-- local pso = gpu:create_pipeline(vscgles, fscgles, {VertexAttribute.VEC2, VertexAttribute.VEC3}, WindingOrder.CLOCKWISE)

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

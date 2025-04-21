-- RUN: cmake --workflow --preset linux-vk-dev && ./artifacts/linux-vk-dev/gpu/gpu-lua

-- buffer experiment
vbo = gpu:create_buffer(BufferUsage.VERTEX, 0, 0, getData())
gpu:destroy_buffer(vbo)

-- shader pipeline experiment

vscglsl = open_file("data/cube.vert.glsl")
vscsprv = compile_file("cube.vert.glsl", ShaderC.VERTEX, vscglsl, true)

fscglsl = open_file("data/cube.frag.glsl")
fscsprv = compile_file("cube.frag.glsl", ShaderC.FRAGMENT, fscglsl, true)

pso = gpu:create_pipeline(vscsprv, fscsprv, {VertexAttribute.VEC3, VertexAttribute.VEC3, VertexAttribute.VEC2})
gpu:destroy_pipeline(pso)

pso = gpu:create_pipeline(vscsprv, fscsprv, {VertexAttribute.VEC3, VertexAttribute.VEC3, VertexAttribute.VEC2})
gpu:destroy_pipeline(pso)
pso = gpu:create_pipeline(vscsprv, fscsprv, {VertexAttribute.VEC3, VertexAttribute.VEC3, VertexAttribute.VEC2})
gpu:destroy_pipeline(pso)
pso = gpu:create_pipeline(vscsprv, fscsprv, {VertexAttribute.VEC3, VertexAttribute.VEC3, VertexAttribute.VEC2})
gpu:destroy_pipeline(pso)
pso = gpu:create_pipeline(vscsprv, fscsprv, {VertexAttribute.VEC3, VertexAttribute.VEC3, VertexAttribute.VEC2})
gpu:destroy_pipeline(pso)

print("Hello, Window!")
while not shouldClose() do
    pollEvents()
end
print("Goodbye, Window!");

vbo = gpu:create_buffer(BufferUsage.VERTEX, 0, 0, getData())

gpu:destroy_buffer(vbo)

print("Hello, Window!")
while not shouldClose() do
    pollEvents()
end
print("Goodbye, Window!");

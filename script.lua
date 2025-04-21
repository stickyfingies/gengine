print("Hello, World!")

window = Window.new(800, 400, "Hello Lua!")

while not window:shouldClose() do
    window:pollEvents()
end

print("Goodbye, World!");

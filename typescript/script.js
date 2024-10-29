// Create a new window
const window = new Window();

// Core game loop
while (!window.shouldClose()) {

    window.update();

    if (window.getEsc()) {
        window.setShouldClose(true);
    }
}

// Adios!
print("Goodbye, JS!");
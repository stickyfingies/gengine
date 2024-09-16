#include "world.h"

#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;

namespace gengine {

class WasmWorld : public World {

	GLFWwindow* window;

public:
	WasmWorld(GLFWwindow* window) : window{window} {
        cout << "Hello, World!" << endl;
    }

	~WasmWorld() {
        cout << "Goodbye, World!" << endl;
    }

	void update(double elapsed_time) override
	{
        if (glfwGetKey(window, GLFW_KEY_SPACE)) {
            cout << "Space" << endl;
        }
		if (glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            cout << "Esc" << endl;
			glfwSetWindowShouldClose(window, true);
		}
	}
};

unique_ptr<World> World::create(GLFWwindow* window, shared_ptr<RenderDevice> renderer)
{
	return make_unique<WasmWorld>(window);
}

} // namespace gengine
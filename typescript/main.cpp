#include <fstream>
#include <iostream>
#include <sstream>

#include <GLFW/glfw3.h>
#include "dukglue/dukglue.h"
#include "duktape.h"

using namespace std;

class Window {
private:
    GLFWwindow* window;
public:
    Window() {
        window = glfwCreateWindow(1280, 720, "Hello, world!", nullptr, nullptr);
    }
    ~Window() {
        glfwDestroyWindow(window);
    }

    void update() {
        glfwPollEvents();
        glfwSwapBuffers(window);
    }
    bool shouldClose() {
        return glfwWindowShouldClose(window);
    }
    void setShouldClose(bool newVal) {
        glfwSetWindowShouldClose(window, newVal);
    }
    bool getEsc() {
        return glfwGetKey(window, GLFW_KEY_ESCAPE);
    }
};

void foo(const char* text) { cout << text << endl; }

int main(int argc, char* argv[])
{
	const char* script_path = "./typescript/script.js";

	ifstream script_file(script_path);
	if (!script_file.is_open()) {
		cerr << "Failed to open " << script_path << endl;
	}
	stringstream script_data;
	script_data << script_file.rdbuf();
	string script_str = script_data.str();

	/* === */

    glfwInit();
	duk_context* ctx = duk_create_heap_default();

	dukglue_register_function(ctx, foo, "print");
    dukglue_register_constructor<Window>(ctx, "Window");
    dukglue_register_method(ctx, &Window::update, "update");
    dukglue_register_method(ctx, &Window::shouldClose, "shouldClose");
    dukglue_register_method(ctx, &Window::setShouldClose, "setShouldClose");
    dukglue_register_method(ctx, &Window::getEsc, "getEsc");

	duk_push_string(ctx, script_str.data());
	const int failed = duk_peval(ctx);
	if (failed) {
		const char* err = duk_safe_to_string(ctx, -1);
        cerr << err << endl;
	}

	duk_destroy_heap(ctx);

    glfwTerminate();

	return 0;
}
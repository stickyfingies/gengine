#include "config.h"
#include <GLFW/glfw3.h>

#include "glm/gtc/matrix_transform.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "assets.h"
#include "camera.hpp"
#include "physics.h"

#include "kernel.h"
#include "gpu.h"
#include "window.h"
#include "world.h"

#include <filesystem>
#include <iostream>
#include <vector>

using namespace gengine;
using namespace std;

extern "C" {
void gengine_print(int i) { cout << "Gengine says " << i << endl; }
}

namespace {

// Bullshit because Emscripten needs a pointer to the main loop
function<void()> loop;
void main_loop() { loop(); }

} // namespace

auto main(int argc, char** argv) -> int
{
	const bool editor_enabled = true;

	EngineKernel* kernel = kernel_create(editor_enabled);

	loop = [&]() -> void { kernel_update(kernel); };

#if GENGINE_PLATFORM_WEB
	emscripten_set_main_loop(main_loop, 0, true);
#else
	while (kernel_running(kernel))
		main_loop();
#endif

	kernel_destroy(kernel);
}

#pragma once

#include <memory>

struct GLFWwindow;

namespace gengine {

struct RenderDevice;

class World {
public:
	static std::unique_ptr<World> create(GLFWwindow* window, std::shared_ptr<RenderDevice> renderer);

	virtual void update(double elapsed_time) = 0;
};

} // namespace gengine
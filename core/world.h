#pragma once

#include <memory>

struct GLFWwindow;

namespace gpu {
struct RenderDevice;
}

class World {
public:
	static std::unique_ptr<World>
	create(std::shared_ptr<GLFWwindow> window, std::shared_ptr<gpu::RenderDevice> renderer);

	virtual ~World() = default;

	virtual void update(double elapsed_time) = 0;
};
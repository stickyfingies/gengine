#pragma once

#include <memory>

struct GLFWwindow;

namespace gengine
{
	class Renderer
	{
	public:
		Renderer(GLFWwindow* window);
		~Renderer();

		auto start_frame()->void;
		auto end_frame()->void;
		auto draw_box(float* transform)->void;
	private:
		struct Impl;
		std::unique_ptr<Impl> impl;
	};
}
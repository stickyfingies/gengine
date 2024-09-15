#include "renderer.h"

#include <GLFW/glfw3.h>

#include <iostream>

namespace gengine {

struct Buffer {};

class RenderDeviceGL : public RenderDevice {
public:

    RenderDeviceGL(GLFWwindow* window) {
        std::cout << "Creating render device" << std::endl;
    }

	auto create_buffer(const BufferInfo& info, const void* data) -> std::unique_ptr<Buffer>
	{
		std::cout << "Creating buffer" << std::endl;
		return nullptr;
	}

	auto destroy_buffer(std::shared_ptr<Buffer> buffer) -> void
	{
		std::cout << "Destroying buffer" << std::endl;
	}

	auto create_image(const ImageAsset& image_asset) -> Image*
	{
		std::cout << "Creating image" << std::endl;
		return nullptr;
	}

	auto destroy_all_images() -> void { std::cout << "Destroying all images" << std::endl; }

	auto create_pipeline(const std::string_view vert_code, const std::string_view frag_code)
		-> ShaderPipeline*
	{
		std::cout << "Creating pipeline" << std::endl;
        return nullptr;
	}

	auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color)
		-> Descriptors*
	{
		std::cout << "Creating descriptors" << std::endl;
        return nullptr;
	}

	auto destroy_pipeline(ShaderPipeline* pso) -> void
	{
		std::cout << "Destroying pipeline" << std::endl;
	}

	auto create_renderable(const GeometryAsset& geometry) -> Renderable
	{
		std::cout << "Creating renderable" << std::endl;
        return {};
	}

	auto render(
		const glm::mat4& view,
		ShaderPipeline* pipeline,
		const std::vector<glm::mat4>& transforms,
		const std::vector<Renderable>& renderables,
		const std::vector<Descriptors*>& descriptors,
		std::function<void()> gui_code) -> void
	{
		//
	}
};

auto RenderDevice::create(GLFWwindow* window) -> std::unique_ptr<RenderDevice>
{
	return std::make_unique<RenderDeviceGL>(window);
}

} // namespace gengine
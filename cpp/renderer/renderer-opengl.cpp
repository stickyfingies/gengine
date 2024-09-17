#include "renderer.h"

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <glad/gl.h>
#endif

#include <iostream>

using namespace std;

namespace gengine {

struct Buffer {
	GLuint VBO;
};

class RenderDeviceGL : public RenderDevice {

	GLFWwindow* window;

public:
	RenderDeviceGL(GLFWwindow* window) : window{window}
	{
		std::cout << "Creating render device" << std::endl;
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);
	}

	auto create_buffer(const BufferInfo& info, const void* data) -> std::unique_ptr<Buffer>
	{
		std::cout << "Creating buffer" << std::endl;
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, info.element_count * info.stride, data, GL_STATIC_DRAW);
		return make_unique<Buffer>(VBO);
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
		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		const auto vertex_source = vert_code.data();
		glShaderSource(vertex_shader, 1, &vertex_source, nullptr);
		glCompileShader(vertex_shader);
		int success;
		char infoLog[512];
		glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
			std::cout << "Vertex shader compilation failed.\n" << infoLog << std::endl;
		}

		GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		const auto fragment_source = frag_code.data();
		glShaderSource(fragment_shader, 1, &fragment_source, nullptr);
		glCompileShader(fragment_shader);
		glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragment_shader, 512, nullptr, infoLog);
			std::cout << "Fragment shader compilation failed.\n" << infoLog << std::endl;
		}

		GLuint shader_program = glCreateProgram();
		glAttachShader(shader_program, vertex_shader);
		glAttachShader(shader_program, fragment_shader);
		glLinkProgram(shader_program);
		glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
			std::cout << "Shader linkage failed.\n" << infoLog << std::endl;
		}

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

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
		glClearColor(0.4, 0.3, 0.8, 1.0);
		glViewport(0, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
};

auto RenderDevice::create(GLFWwindow* window) -> std::unique_ptr<RenderDevice>
{
	return std::make_unique<RenderDeviceGL>(window);
}

auto RenderDevice::configure_glfw() -> void
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
}

} // namespace gengine
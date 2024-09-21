#include "gpu.h"

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

struct gpu::Buffer {
	GLuint gl_buffer;
};

struct gpu::ShaderPipeline {
	GLuint gl_program;
};

struct gpu::Geometry {
	GLuint vao;
	shared_ptr<gpu::Buffer> vbo;
	shared_ptr<gpu::Buffer> ebo;
	unsigned long index_count;
};

// This abstracts over OpenGL/glES differences
namespace webgl {

void genVertexArrays(GLuint count, GLuint* vao)
{
#ifdef __EMSCRIPTEN__
	glGenVertexArraysOES(1, vao);
#else
	glGenVertexArrays(1, vao);
#endif
}

void bindVertexArray(GLuint vao)
{
#ifdef __EMSCRIPTEN__
	glBindVertexArrayOES(vao);
#else
	glBindVertexArray(vao);
#endif
}

} // namespace webgl

namespace gpu {

class RenderDeviceGL : public RenderDevice {

	GLFWwindow* window;

	const array<GLint, 2> BUFFER_USAGE_TABLE{GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER};

public:
	RenderDeviceGL(GLFWwindow* window) : window{window}
	{
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);

		const GLubyte* vendor = glGetString(GL_VENDOR);
		const GLubyte* hardware = glGetString(GL_RENDERER);
		const GLubyte* version = glGetString(GL_VERSION);

		cout << "Render driver: " << version << endl;
		cout << "Render platform: " << vendor << " on " << hardware << endl;
	}

	auto create_buffer(const BufferInfo& info, const void* data) -> std::unique_ptr<Buffer> override
	{
		const auto size = info.element_count * info.stride;

		const auto buffer_type = BUFFER_USAGE_TABLE[static_cast<unsigned int>(info.usage)];

		std::cout << "Buffer size: " << size << std::endl;
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(buffer_type, VBO);
		glBufferData(buffer_type, size, data, GL_STATIC_DRAW);
		return make_unique<Buffer>(VBO);
	}

	auto destroy_buffer(shared_ptr<Buffer> buffer) -> void override
	{
		//
		std::cout << "Destroying buffer" << std::endl;
	}

	auto create_image(const gengine::ImageAsset& image_asset) -> Image* override
	{
		std::cout << "Creating image" << std::endl;
		return nullptr;
	}

	auto destroy_all_images() -> void override { cout << "Destroying all images" << endl; }

	auto create_pipeline(const string_view vert_code, const string_view frag_code)
		-> ShaderPipeline* override
	{
		// Reserved for GL error strings
		int success;
		char infoLog[512];

		// Compile vertex shader
		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		const auto vertex_source = vert_code.data();
		glShaderSource(vertex_shader, 1, &vertex_source, nullptr);
		glCompileShader(vertex_shader);
		glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(vertex_shader, 512, nullptr, infoLog);
			cout << "Error: Pipeline creation failed." << endl;
			cout << "Reason: Vertex shader compilation failed." << endl;
			cout << infoLog << endl;
		}

		// Compile fragment shader
		GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		const auto fragment_source = frag_code.data();
		glShaderSource(fragment_shader, 1, &fragment_source, nullptr);
		glCompileShader(fragment_shader);
		glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
		if (!success) {
			glGetShaderInfoLog(fragment_shader, 512, nullptr, infoLog);
			cout << "Fragment shader compilation failed.\n" << infoLog << endl;
		}

		// Link shader stages
		GLuint shader_program = glCreateProgram();
		glAttachShader(shader_program, vertex_shader);
		glAttachShader(shader_program, fragment_shader);
		glLinkProgram(shader_program);
		glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shader_program, 512, nullptr, infoLog);
			cout << "Shader linkage failed.\n" << infoLog << endl;
		}
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		cout << "Pipeline" << endl;
		const auto pipeline = new ShaderPipeline{shader_program};
		return pipeline;
	}

	auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color)
		-> Descriptors* override
	{
		std::cout << "Creating descriptors" << std::endl;
		return nullptr;
	}

	auto destroy_pipeline(ShaderPipeline* pso) -> void override
	{
		glDeleteProgram(pso->gl_program);
		delete pso;
		std::cout << "Destroyed pipeline" << std::endl;
	}

	auto create_geometry(const gengine::GeometryAsset& geometry) -> Geometry* override
	{
		std::cout << "Creating geometry" << std::endl;

		GLuint vao;
		webgl::genVertexArrays(1, &vao);
		webgl::bindVertexArray(vao);

		const auto& vertices = geometry.vertices;
		const auto& vertices_aux = geometry.vertices_aux;
		const auto& indices = geometry.indices;

		auto gpu_data = std::vector<float>{};
		for (int i = 0; i < vertices.size() / 3; i++) {
			const auto v = (i * 3);
			gpu_data.push_back(vertices[v + 0]);
			gpu_data.push_back(-vertices[v + 1]);
			gpu_data.push_back(vertices[v + 2]);
			const auto a = (i * 5);
			gpu_data.push_back(vertices_aux[a + 0]);
			gpu_data.push_back(vertices_aux[a + 1]);
			gpu_data.push_back(vertices_aux[a + 2]);
			gpu_data.push_back(vertices_aux[a + 3]);
			gpu_data.push_back(vertices_aux[a + 4]);
		}

		auto vbo = create_buffer(
			{BufferInfo::Usage::VERTEX, sizeof(float), vertices.size()}, vertices.data());
		auto ebo = create_buffer(
			{BufferInfo::Usage::INDEX, sizeof(unsigned int), indices.size()}, indices.data());

		// TODO - see the definition of `Vertex` in gpu.vulkan.cpp
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		return new Geometry{vao, std::move(vbo), std::move(ebo), indices.size()};
	}

	auto destroy_geometry(const Geometry* geometry) -> void override
	{
		destroy_buffer(geometry->vbo);
		destroy_buffer(geometry->vbo);
		delete geometry;
	}

	auto render(
		const glm::mat4& view,
		ShaderPipeline* pipeline,
		const vector<glm::mat4>& transforms,
		const vector<Geometry*>& renderables,
		const vector<Descriptors*>& descriptors,
		function<void()> gui_code) -> void override
	{
		glClearColor(0.4, 0.3, 0.8, 1.0);
		glViewport(0, 0, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(pipeline->gl_program);

		for (const auto& mesh : renderables) {
			webgl::bindVertexArray(mesh->vao);
			glDrawElements(GL_TRIANGLES, mesh->index_count, GL_UNSIGNED_INT, 0);
		}

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

} // namespace gpu
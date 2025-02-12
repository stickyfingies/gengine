#include "gpu.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include <glad/glad.h>
#endif

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

using namespace std;

struct gpu::Buffer {
	gpu::BufferInfo info;
	GLuint gl_buffer;
};

struct gpu::ShaderPipeline {
	GLuint gl_program;
	std::vector<gpu::VertexAttribute> vertex_attributes;
};

struct gpu::Image {
	GLuint gl_texture;
};

struct gpu::Descriptors {
	gpu::Image* albedo;
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
	glGenVertexArraysOES(count, vao);
#else
	glGenVertexArrays(count, vao);
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

void GLAPIENTRY messageCallback(
	GLenum _source,
	GLenum type,
	GLuint _id,
	GLenum _severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	cout << "GL: (" << type << ") " << message << endl;
}

bool init_gl()
{
#ifndef __EMSCRIPTEN__
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "Error: Failed to initialize GLAD" << endl;
		return false;
	}
	// glEnable(GL_DEBUG_OUTPUT);
	// glDebugMessageCallback(webgl::messageCallback, 0);
#endif
	return true;
}

} // namespace webgl

void glfw_framebuffer_size_cb(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

namespace gpu {

class RenderDeviceGL : public RenderDevice {

	GLFWwindow* window;

	const array<GLint, 2> BUFFER_USAGE_TABLE{GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER};

public:
	RenderDeviceGL(GLFWwindow* window) : window{window}
	{
		glfwMakeContextCurrent(window);
		glfwSwapInterval(1);
		glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_cb);

		cout << "RenderDeviceGL" << endl;

		const auto gl_success = webgl::init_gl();
		if (!gl_success)
			return;

		const GLubyte* vendor = glGetString(GL_VENDOR);
		const GLubyte* hardware = glGetString(GL_RENDERER);
		const GLubyte* version = glGetString(GL_VERSION);

		cout << "Render driver: " << version << endl;
		cout << "Render platform: " << vendor << " on " << hardware << endl;

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		glFrontFace(GL_CCW);
	}

	~RenderDeviceGL() { cout << "~ RenderDeviceGL" << endl; }

	auto create_buffer(const BufferInfo& info, const void* data) -> Buffer* override
	{
		const auto size = info.element_count * info.stride;

		const auto buffer_type = BUFFER_USAGE_TABLE[static_cast<unsigned int>(info.usage)];

		cout << "GPU Buffer size: " << size << " stride: " << info.stride << " type: 0x" << hex
			 << buffer_type << dec << endl;
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(buffer_type, VBO);
		glBufferData(buffer_type, size, data, GL_STATIC_DRAW);
		return new Buffer{info, VBO};
	}

	auto destroy_buffer(Buffer* buffer) -> void override
	{
		cout << "~ GPU Buffer " << buffer << endl;
		glDeleteBuffers(1, &buffer->gl_buffer);
		delete buffer;
	}

	auto create_image(
		const std::string& name, int width, int height, int channel_count, unsigned char* data_in)
		-> Image* override
	{
		GLuint texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		// const float tex_border_color[] = {1.0f, 1.0f, 0.0f, 1.0f};
		// glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, tex_border_color);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data_in);
		glGenerateMipmap(GL_TEXTURE_2D);

		const auto image = new Image{texture};

		cout << "GPU Image " << name << " " << image << endl;

		return image;
	}

	auto destroy_all_images() -> void override { cout << "Destroying all images" << endl; }

	auto create_pipeline(
		string_view vert_code,
		string_view frag_code,
		const vector<VertexAttribute>& vertex_attributes) -> ShaderPipeline* override
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
			// abort();
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
			// abort();
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
			// abort();
		}
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		const auto pipeline = new ShaderPipeline{shader_program, vertex_attributes};

		cout << "Pipeline " << pipeline << endl;

		return pipeline;
	}

	auto create_descriptors(ShaderPipeline* pipeline, Image* albedo, const glm::vec3& color)
		-> Descriptors* override
	{
		const auto descriptor = new Descriptors{albedo};

		cout << "GPU Descriptor " << descriptor << endl;

		return descriptor;
	}

	auto destroy_pipeline(ShaderPipeline* pso) -> void override
	{
		cout << "Destroying pipeline " << pso << endl;
		glDeleteProgram(pso->gl_program);
		delete pso;
	}

	auto create_geometry(ShaderPipeline* pipeline, Buffer* vertex_buffer, Buffer* index_buffer)
		-> Geometry* override
	{
		std::cout << "Creating geometry" << std::endl;

		GLuint vao;
		webgl::genVertexArrays(1, &vao);

		// Bind vao for the remainder of this function
		webgl::bindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->gl_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer->gl_buffer);

		// TODO - see the definition of `Vertex` in gpu.vulkan.cpp
		size_t vertex_size = 0;
		const size_t attribute_count = pipeline->vertex_attributes.size();
		// First, calculate the size of one vertex
		for (size_t attribute_idx = 0; attribute_idx < attribute_count; attribute_idx++) {
			const auto attribute = pipeline->vertex_attributes.at(attribute_idx);
			if (attribute == VertexAttribute::VEC3_FLOAT)
				vertex_size += 3 * sizeof(float);
			else if (attribute == VertexAttribute::VEC2_FLOAT)
				vertex_size += 2 * sizeof(float);
		}
		// Next, generate the gl vertex attributes
		size_t attribute_offset = 0;
		for (size_t attribute_idx = 0; attribute_idx < attribute_count; attribute_idx++) {
			const auto attribute = pipeline->vertex_attributes.at(attribute_idx);
			if (attribute == VertexAttribute::VEC3_FLOAT) {
				glVertexAttribPointer(
					attribute_idx, 3, GL_FLOAT, GL_FALSE, vertex_size, (void*)(attribute_offset));
				glEnableVertexAttribArray(attribute_idx);
				attribute_offset += 3 * sizeof(float);
			}
			else if (attribute == VertexAttribute::VEC2_FLOAT) {
				glVertexAttribPointer(
					attribute_idx, 2, GL_FLOAT, GL_FALSE, vertex_size, (void*)(attribute_offset));
				glEnableVertexAttribArray(attribute_idx);
				attribute_offset += 2 * sizeof(float);
			}
		}

		const auto index_count = index_buffer->info.element_count;
		const auto gpu_geometry = new Geometry{
			vao,
			std::unique_ptr<Buffer>(vertex_buffer),
			std::unique_ptr<Buffer>(index_buffer),
			index_count};

		cout << "GPU Geometry indices: " << index_count << " " << gpu_geometry << endl;

		return gpu_geometry;
	}

	auto destroy_geometry(const Geometry* geometry) -> void override
	{
		destroy_buffer(geometry->vbo.get());
		destroy_buffer(geometry->ebo.get());
		delete geometry;
	}

	auto render(
		const glm::mat4& view,
		ShaderPipeline* pipeline,
		const vector<glm::mat4>& transforms,
		const vector<Geometry*>& geometries,
		const vector<Descriptors*>& descriptors,
		function<void()> gui_code) -> void override
	{
		assert(transforms.size() == geometries.size());
		assert(transforms.size() == descriptors.size());

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) {
			cout << "GL Error: " << err << endl;
		}

		glClearColor(0.4, 0.3, 0.8, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(pipeline->gl_program);

		auto proj = glm::perspective(glm::radians(90.0f), 0.8888f, 0.1f, 10000.0f);
		const GLint u_projection = glGetUniformLocation(pipeline->gl_program, "projection");
		glUniformMatrix4fv(u_projection, 1, GL_FALSE, glm::value_ptr(proj));
		const GLint u_view = glGetUniformLocation(pipeline->gl_program, "view");
		glUniformMatrix4fv(u_view, 1, GL_FALSE, glm::value_ptr(view));

		for (auto i = 0ul; i < transforms.size(); i++) {
			const auto& matrix = transforms[i];
			const auto& geometry = geometries[i];
			const auto& descriptor = descriptors[i];

			const GLint u_model = glGetUniformLocation(pipeline->gl_program, "model");
			glUniformMatrix4fv(u_model, 1, GL_FALSE, glm::value_ptr(matrix));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, descriptor->albedo->gl_texture);
			const auto location = glGetUniformLocation(pipeline->gl_program, "tDiffuse");
			glUniform1i(location, 0);

			webgl::bindVertexArray(geometry->vao);
			glDrawElements(GL_TRIANGLES, geometry->index_count, GL_UNSIGNED_INT, 0);
		}
	}
};

auto RenderDevice::create(GLFWwindow* window) -> std::unique_ptr<RenderDevice>
{
	return std::make_unique<RenderDeviceGL>(window);
}

auto configure_glfw() -> void
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	cout << "Configured GLFW" << endl;
}

} // namespace gpu
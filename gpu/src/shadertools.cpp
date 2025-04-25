/**
 * BUILD: cmake --workflow --preset linux-(vk|gl)-dev
 * RUN: ./artifacts/bin/linux-(vk|gl)-dev/gpu/shadertools
 */

#include "gpu.h"
#include "shaders.h"

#include <iostream>

#include <rfl.hpp>
#include <rfl/json.hpp>

using namespace std;

static bool process_glsl_file(const string& filename_base, gpu::ShaderStage stage)
{
	const string glsl_filename = filename_base + ".glsl";
	const string sprv_filename = filename_base + ".spv";
	const string gles_filename = filename_base + ".glsles";

	// Open the GLSL file
	ifstream glsl_shaderfile(glsl_filename);
	if (!glsl_shaderfile.is_open()) {
		cerr << "Error opening file: " << glsl_filename << endl;
		return false;
	}
	const string glsl_shader_code(
		(istreambuf_iterator<char>(glsl_shaderfile)), istreambuf_iterator<char>());
	glsl_shaderfile.close();

	// Compile the GLSL shader to SPIR-V
	const auto spirv_blob =
		gpu::glsl_to_sprv(glsl_filename, stage, glsl_shader_code, false);

    // Write the SPIR-V blob to a file
	ofstream spirv_shaderfile(sprv_filename, ios::binary | ios::trunc);
	if (!spirv_shaderfile.is_open()) {
		cerr << "Error opening file: " << sprv_filename << endl;
		return false;
	}
	spirv_shaderfile.write(
		reinterpret_cast<const char*>(spirv_blob.data()), spirv_blob.size() * sizeof(uint32_t));
	spirv_shaderfile.close();

    // Convert the SPIR-V blob to GLSL ES
    const auto gles_blob = gpu::sprv_to_gles(spirv_blob);

    // Write the GLSL ES blob to a file
    ofstream gles_shaderfile(gles_filename, ios::binary | ios::trunc);
    if (!gles_shaderfile.is_open()) {
        cerr << "Error opening file: " << gles_filename << endl;
        return false;
    }
    gles_shaderfile.write(
        reinterpret_cast<const char*>(gles_blob.data()), gles_blob.size() * sizeof(uint32_t));
    gles_shaderfile.close();

    return true;
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		cerr << "Usage: " << argv[0] << " <shader_file>" << endl;
		return 1;
	}

	string shader_file = argv[1];

    bool success;

	success = process_glsl_file(shader_file + ".vert", gpu::ShaderStage::VERTEX);
    if (!success) {
        cerr << "Error processing vertex shader file: " << shader_file << endl;
        return 1;
    }

    success = process_glsl_file(shader_file + ".frag", gpu::ShaderStage::FRAGMENT);
    if (!success) {
        cerr << "Error processing fragment shader file: " << shader_file << endl;
        return 1;
    }

	// write json to file and erase current file
	// const string json = rfl::json::write(shader_object.vertex_attributes);
	// ofstream json_file(shader_file + "_.json", ofstream::out | ofstream::trunc);
	// if (!json_file.is_open()) {
	//     cerr << "Error opening file: " << shader_file << ".json" << endl;
	//     return 1;
	// }
	// json_file << json;
	// json_file.close();

	cout << "Shader compiled successfully!" << endl;

	return 0;
}
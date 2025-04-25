#include "shaders.h"

#include <shaderc/shaderc.hpp>
#include <spirv_glsl.hpp>
#include <spirv_reflect.h>

#include <iostream>

#ifndef GPU_BACKEND
#define GPU_BACKEND "Undefined"
#endif

using namespace std;
using namespace gpu;

// Structure to hold the layout information for a single member
struct BufferMemberLayout {
	std::string name;
	uint32_t offset; // Offset from the start of the uniform buffer block in bytes
	uint32_t size;	 // Size of the member in bytes
					 // You could add more information here, like type flags, array dimensions, etc.
};

// Structure to hold the layout information for the entire uniform buffer block
struct UniformBufferLayout {
	uint32_t set;
	uint32_t binding;
	std::string block_name; // The name of the uniform block (e.g., "CameraParams")
	uint32_t block_size;	// Total size of the block in bytes (including padding)
	std::vector<gpu::BufferMemberLayout> members;
};

// Structure to hold the layout information for a push constant block
struct PushConstantLayout {
	std::string block_name; // Often "$push_constants" or the name if defined in GLSL/HLSL
	SpvReflectShaderStageFlagBits stage_flags; // Which shader stages use this block
	uint32_t size;							   // Total size of the push constant block in bytes
	std::vector<gpu::BufferMemberLayout> members;
};

struct VertexAttributeLayout {
	uint32_t location;
	uint32_t size; // in bytes
	std::string name;
	std::string format;
};

// Helper to convert SpvReflectFormat to string for printing (optional)
static std::string spvFormatToString(SpvReflectFormat format)
{
	switch (format) {
	case SPV_REFLECT_FORMAT_UNDEFINED:
		return "UNDEFINED";
	case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
		return "R32G32B32A32_UINT";
	case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
		return "R32G32B32A32_SINT";
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
		return "R32G32B32A32_SFLOAT";
	case SPV_REFLECT_FORMAT_R32G32B32_UINT:
		return "R32G32B32_UINT";
	case SPV_REFLECT_FORMAT_R32G32B32_SINT:
		return "R32G32B32_SINT";
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
		return "R32G32B32_SFLOAT";
	case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
		return "R16G16B16A16_UINT";
	case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
		return "R16G16B16A16_SINT";
	case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
		return "R16G16B16A16_SFLOAT";
	case SPV_REFLECT_FORMAT_R32G32_UINT:
		return "R32G32_UINT";
	case SPV_REFLECT_FORMAT_R32G32_SINT:
		return "R32G32_SINT";
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
		return "R32G32_SFLOAT";
	case SPV_REFLECT_FORMAT_R16G16_UINT:
		return "R16G16_UINT";
	case SPV_REFLECT_FORMAT_R16G16_SINT:
		return "R16G16_SINT";
	case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
		return "R16G16_SFLOAT";
	case SPV_REFLECT_FORMAT_R32_UINT:
		return "R32_UINT";
	case SPV_REFLECT_FORMAT_R32_SINT:
		return "R32_SINT";
	case SPV_REFLECT_FORMAT_R32_SFLOAT:
		return "R32_SFLOAT";
	case SPV_REFLECT_FORMAT_R16_UINT:
		return "R16_UINT";
	case SPV_REFLECT_FORMAT_R16_SINT:
		return "R16_SINT";
	case SPV_REFLECT_FORMAT_R16_SFLOAT:
		return "R16_SFLOAT";
	default:
		return "UNKNOWN";
	}
}

static uint32_t spvFormatToSize(SpvReflectFormat format)
{
	switch (format) {
	case SPV_REFLECT_FORMAT_UNDEFINED:
		return 0;
	case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
		return 16;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
		return 16;
	case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
		return 16;
	case SPV_REFLECT_FORMAT_R32G32B32_UINT:
		return 12;
	case SPV_REFLECT_FORMAT_R32G32B32_SINT:
		return 12;
	case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:
		return 8;
	case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:
		return 8;
	case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT:
		return 8;
	case SPV_REFLECT_FORMAT_R32G32_UINT:
		return 8;
	case SPV_REFLECT_FORMAT_R32G32_SINT:
		return 8;
	case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
		return 8;
	case SPV_REFLECT_FORMAT_R16G16_UINT:
		return 4;
	case SPV_REFLECT_FORMAT_R16G16_SINT:
		return 4;
	case SPV_REFLECT_FORMAT_R16G16_SFLOAT:
		return 4;
	case SPV_REFLECT_FORMAT_R32_UINT:
		return 4;
	case SPV_REFLECT_FORMAT_R32_SINT:
		return 4;
	case SPV_REFLECT_FORMAT_R32_SFLOAT:
		return 4;
	case SPV_REFLECT_FORMAT_R16_UINT:
		return 2;
	case SPV_REFLECT_FORMAT_R16_SINT:
		return 2;
	case SPV_REFLECT_FORMAT_R16_SFLOAT:
		return 2;
	default:
		return 0;
	}
}

/*
 * @brief Finds the vertex input attribute layout required by a vertex shader.
 *
 * @param[in] spirv_data Pointer to the SPIR-V binary data of a vertex shader.
 * @param[in] data_size Size of the SPIR-V binary data in bytes.
 * @param[out] out_layout A vector that will be populated with the layout information
 * for each vertex input attribute found, sorted by location.
 *
 * @return True if successful, false otherwise (e.g., not a vertex shader, no inputs found).
 */
static bool getVertexInputLayout(
	const uint32_t* spirv_data, size_t data_size, std::vector<VertexAttributeLayout>& out_layout)
{
	SpvReflectShaderModule module;
	SpvReflectResult result = spvReflectCreateShaderModule(data_size, spirv_data, &module);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to create SPIR-V reflection module: " << result << std::endl;
		return false;
	}

	// Ensure the module is destroyed when the function exits
	auto cleanup = [&]() { spvReflectDestroyShaderModule(&module); };

	// Optional: Check if it's actually a vertex shader
	if (module.shader_stage != SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) {
		std::cerr << "Error: Provided SPIR-V is not a vertex shader (stage: " << module.shader_stage
				  << ")" << std::endl;
		cleanup();
		return false;
	}

	uint32_t count = 0;
	result = spvReflectEnumerateInputVariables(&module, &count, nullptr);
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to enumerate input variables count: " << result << std::endl;
		cleanup();
		return false;
	}

	if (count == 0) {
		std::cout << "Info: No input variables found in the vertex shader." << std::endl;
		// This is unusual for a vertex shader, but possible.
		cleanup();
		return true; // Indicate success, but with no inputs
	}

	std::vector<SpvReflectInterfaceVariable*> input_vars(count);
	result = spvReflectEnumerateInputVariables(&module, &count, input_vars.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to enumerate input variables: " << result << std::endl;
		cleanup();
		return false;
	}

	for (uint32_t i = 0; i < count; ++i) {
		const auto* var = input_vars[i];

		// Skip built-in variables (like gl_VertexIndex, gl_InstanceIndex)
		if (var->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
			continue;
		}

		// Also expect a type description to get size/format, though location implies a type
		if (!var->type_description) {
			std::cerr << "Warning: Input variable '" << (var->name ? var->name : "Unnamed")
					  << "' does not have a TypeDescription. Skipping." << std::endl;
			continue;
		}

		out_layout.push_back(
			{.location = var->location,
			 .size = spvFormatToSize(var->format),				 // Size of the attribute
			 .name = var->name ? var->name : "UnnamedAttribute", // Use name if available
			 .format = spvFormatToString(var->format)});		 // Size of the attribute
	}

	// Sort the attributes by location for consistent ordering
	std::sort(
		out_layout.begin(),
		out_layout.end(),
		[](const VertexAttributeLayout& a, const VertexAttributeLayout& b) {
			return a.location < b.location;
		});

	cleanup();
	return true; // Indicate success
}

/*
 * @brief Finds the memory layout of the push constant block within a SPIR-V module.
 *
 * @param[in] spirv_data Pointer to the SPIR-V binary data.
 * @param[in] data_size Size of the SPIR-V binary data in bytes.
 * @param[out] out_layout The layout information for the push constant block if found.
 *
 * @return True if a push constant block was found and its layout extracted, false otherwise.
 * Returns false if no push constant block is present.
 */
static bool
getPushConstantLayout(const uint32_t* spirv_data, size_t data_size, PushConstantLayout& out_layout)
{
	SpvReflectShaderModule module;
	SpvReflectResult result = spvReflectCreateShaderModule(data_size, spirv_data, &module);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to create SPIR-V reflection module: " << result << std::endl;
		return false;
	}

	// Ensure the module is destroyed when the function exits
	auto cleanup = [&]() { spvReflectDestroyShaderModule(&module); };

	uint32_t count = 0;
	result = spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to enumerate push constant blocks count: " << result
				  << std::endl;
		cleanup();
		return false;
	}

	if (count == 0) {
		// This is a valid scenario - the shader simply has no push constants.
		std::cout << "Info: No push constant blocks found in this shader module." << std::endl;
		cleanup();
		return false; // Indicate that no block was processed
	}

	// In Vulkan, push constants from different OpVariable instructions within
	// a single module are typically merged into a single conceptual block.
	// spirv-reflect will usually report a single block covering all usages.
	// We'll take the first one reported.
	std::vector<SpvReflectBlockVariable*> pc_blocks(count);
	result = spvReflectEnumeratePushConstantBlocks(&module, &count, pc_blocks.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to enumerate push constant blocks: " << result << std::endl;
		cleanup();
		return false;
	}

	// Process the first push constant block found
	const auto& pc_block = *pc_blocks[0];

	out_layout.block_name = pc_block.name ? pc_block.name : "$push_constants"; // Default name
	// out_layout.stage_flags = pc_block.stage_flags;
	out_layout.size = pc_block.size;

	for (uint32_t i = 0; i < pc_block.member_count; ++i) {
		const auto& member = pc_block.members[i];
		out_layout.members.push_back(
			{member.name ? member.name : "UnnamedMember", // Use member name if available
			 member.offset,
			 member.size});
	}

	if (count > 1) {
		std::cout << "Warning: Found " << count
				  << " push constant blocks. Processing only the first."
				  << " This is often due to internal compiler structure; Vulkan typically presents "
					 "them as one block."
				  << std::endl;
	}

	cleanup();
	return true; // Indicate that a block was found and processed
}

/*
 * @brief Finds the memory layout of a specific uniform buffer within a SPIR-V module.
 *
 * @param[in] spirv_data Pointer to the SPIR-V binary data.
 * @param[in] data_size Size of the SPIR-V binary data in bytes.
 * @param[in] target_set The descriptor set number of the target uniform buffer.
 * @param[in] target_binding The binding number of the target uniform buffer.
 * @param[out] out_layout A vector that will be populated with the layout information
 * for each member of the uniform buffer block if found.
 *
 * @return True if the uniform buffer was found and its layout extracted, false otherwise.
 */
static bool getUniformBufferLayout(
	const uint32_t* spirv_data,
	size_t data_size,
	uint32_t target_set,
	uint32_t target_binding,
	gpu::UniformBufferLayout& out_layout)
{
	SpvReflectShaderModule module;
	SpvReflectResult result = spvReflectCreateShaderModule(data_size, spirv_data, &module);

	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to create SPIR-V reflection module: " << result << std::endl;
		return false;
	}

	// Ensure the module is destroyed when the function exits
	auto cleanup = [&]() { spvReflectDestroyShaderModule(&module); };

	uint32_t count = 0;
	result = spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to enumerate descriptor bindings count: " << result
				  << std::endl;
		cleanup();
		return false;
	}

	std::vector<SpvReflectDescriptorBinding*> bindings(count);
	result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());
	if (result != SPV_REFLECT_RESULT_SUCCESS) {
		std::cerr << "Error: Failed to enumerate descriptor bindings: " << result << std::endl;
		cleanup();
		return false;
	}

	const SpvReflectDescriptorBinding* found_binding = nullptr;

	// Find the specific uniform buffer binding
	for (uint32_t i = 0; i < count; ++i) {
		const auto* binding = bindings[i];
		if (binding->set == target_set && binding->binding == target_binding &&
			binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
			found_binding = binding;
			break;
		}
	}

	if (!found_binding) {
		std::cerr << "Error: Uniform buffer with set " << target_set << " and binding "
				  << target_binding << " not found." << std::endl;
		cleanup();
		return false;
	}

	// Extract layout information
	const SpvReflectBlockVariable& block = found_binding->block;

	out_layout.set = found_binding->set;
	out_layout.binding = found_binding->binding;
	out_layout.block_name = block.name ? block.name : ""; // Use block name if available
	out_layout.block_size = block.size;					  // Total size of the block

	for (uint32_t i = 0; i < block.member_count; ++i) {
		const auto& member = block.members[i];
		out_layout.members.push_back(
			{member.name ? member.name : "UnnamedMember", // Use member name if available
			 member.offset,
			 member.size});
	}

	cleanup();
	return true;
}

// Convert SPIRV to GLSL ES 300 (WebGL 2)
std::vector<uint32_t> gpu::sprv_to_gles(std::vector<uint32_t> spirv_binary)
{
	spirv_cross::CompilerGLSL glsl(std::move(spirv_binary));

	// Set some options.
	spirv_cross::CompilerGLSL::Options options;
	options.version = 300;
	options.es = true;
	glsl.set_common_options(options);

	// Compile the SPIRV to GLSL.
	std::string source = glsl.compile();

	// convert source to vector of uint32_t
	std::vector<uint32_t> glsl_binary;
	glsl_binary.resize(source.size() / sizeof(uint32_t));
	for (size_t i = 0; i < source.size() / sizeof(uint32_t); ++i) {
		glsl_binary[i] = *reinterpret_cast<const uint32_t*>(source.data() + i * sizeof(uint32_t));
	}

	return glsl_binary;
}

// Compiles a shader to a SPIR-V binary. Returns the binary as
// a vector of 32-bit words.
std::vector<uint32_t> gpu::glsl_to_sprv(
	const std::string& source_name,
	gpu::ShaderStage stage,
	const std::string& source,
	bool optimize)
{
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) {
		options.SetOptimizationLevel(shaderc_optimization_level_size);
	}

	// Add this line to preserve uniform names
	options.SetGenerateDebugInfo();

	shaderc_shader_kind shader_kind = stage == ShaderStage::VERTEX
										  ? shaderc_shader_kind::shaderc_glsl_vertex_shader
										  : shaderc_shader_kind::shaderc_glsl_fragment_shader;

	shaderc::SpvCompilationResult module =
		compiler.CompileGlslToSpv(source, shader_kind, source_name.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		std::cerr << module.GetErrorMessage();
		return std::vector<uint32_t>();
	}

	std::vector<uint32_t> spirv_blob = {module.cbegin(), module.cend()};

	std::vector<VertexAttributeLayout> vertex_layout;
	bool success = getVertexInputLayout(
		spirv_blob.data(),
		spirv_blob.size() * sizeof(uint32_t), // size in bytes!
		vertex_layout);

	if (success) {
		if (vertex_layout.empty()) {
			std::cout << "Vertex shader has no user-defined input attributes." << std::endl;
		}
		else {
			std::cout << "Vertex Input Layout:" << std::endl;
			for (const auto& attr : vertex_layout) {
				std::cout << "  - Location: " << attr.location << ", Name: " << attr.name
						  << ", Format: " << attr.format << ", Size: " << attr.size << " bytes"
						  << std::endl;
			}
		}

		// --- How you would use this for API setup (Vulkan/OpenGL) ---
		// This layout information is used to configure the vertex input stage
		// in your graphics API pipeline.

		// In Vulkan, you'd use this to create VkVertexInputAttributeDescription
		// and VkVertexInputBindingDescription structures.

		// Example (Vulkan):
		// Assuming all attributes come from a single buffer (binding 0)
		//
		// VkVertexInputBindingDescription bindingDescription{};
		// bindingDescription.binding = 0;
		// bindingDescription.stride = /* Calculate total size of one vertex */;
		// bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // or INSTANCE
		//
		// std::vector<VkVertexInputAttributeDescription>
		// attributeDescriptions(vertex_layout.size()); uint32_t currentOffset = 0; for (size_t i =
		// 0; i < vertex_layout.size(); ++i) {
		//     attributeDescriptions[i].binding = 0; // Match the binding description
		//     attributeDescriptions[i].location = vertex_layout[i].location;
		//     attributeDescriptions[i].format =
		//     mapSpvReflectFormatToVulkan(vertex_layout[i].format); // <-- Need a mapping function
		//     attributeDescriptions[i].offset = currentOffset; // Offset within the vertex buffer
		//     stride
		//
		//     // For tightly packed vertex data:
		//     currentOffset += vertex_layout[i].size;
		//
		//     // If using std140/std430-like alignment in vertex buffer (less common for vertex
		//     data):
		//     // You'd need reflection data on type alignment and calculate padded offset.
		//     // But usually vertex buffers are tightly packed per attribute.
		// }
		// // bindingDescription.stride = currentOffset; // This is the stride if tightly packed
		//
		// // These descriptions go into the VkPipelineVertexInputStateCreateInfo

		// In OpenGL, you'd use this with glVertexAttribPointer:
		//
		// GLuint binding_point = 0; // Corresponds to Vulkan binding index
		// GLsizei stride = /* Calculate total size of one vertex */;
		//
		// for (const auto& attr : vertex_layout) {
		//    // Need a mapping from SpvReflectFormat to GLenum type, components, and normalization
		//    bool GLenum type; int components; GLboolean normalized; // Get these from mapping
		//    function void* offset_ptr = (void*)(uintptr_t)/* Calculate offset for this attribute
		//    */;
		//
		//    glVertexAttribPointer(
		//        attr.location, // Use the location from reflection
		//        components,    // e.g., 3 for vec3
		//        type,          // e.g., GL_FLOAT
		//        normalized,    // e.g., GL_FALSE for float/int
		//        stride,        // Total size of one vertex
		//        offset_ptr     // Byte offset from start of vertex in buffer
		//    );
		//    glEnableVertexAttribArray(attr.location);
		//    // If using glBindVertexBuffer (OpenGL 4.3+):
		//    // glVertexAttribBinding(attr.location, binding_point);
		// }
		// glBindVertexBuffer(binding_point, VBO, 0, stride); // Bind VBO to binding point
	}

	uint32_t target_set = 0;
	uint32_t target_binding = 0;

	gpu::UniformBufferLayout ubo_layout;
	success = getUniformBufferLayout(
		spirv_blob.data(),
		spirv_blob.size() * sizeof(uint32_t), // size in bytes!
		target_set,
		target_binding,
		ubo_layout);

	if (success) {
		std::cout << "Successfully found uniform buffer:" << std::endl;
		std::cout << "  Set: " << ubo_layout.set << std::endl;
		std::cout << "  Binding: " << ubo_layout.binding << std::endl;
		std::cout << "  Block Name: " << ubo_layout.block_name << std::endl;
		std::cout << "  Block Size (bytes): " << ubo_layout.block_size << std::endl;
		std::cout << "  Members:" << std::endl;
		for (const auto& member : ubo_layout.members) {
			std::cout << "    - Name: " << member.name << ", Offset: " << member.offset << " bytes"
					  << ", Size: " << member.size << " bytes" << std::endl;
		}

		// --- How you would use this for memcpy ---
		// Assume you have a buffer allocated for this UBO, maybe using Vulkan/OpenGL APIs
		// uint8_t* ubo_memory_ptr = ... // Pointer to the mapped buffer memory

		// Example: Copying a float 'time' member
		// float current_time = 1.23f;
		// For a member named "time", find its layout
		// auto it = std::find_if(ubo_layout.members.begin(), ubo_layout.members.end(),
		//                       [](const UniformBufferMemberLayout& m){ return m.name == "time";
		//                       });
		// if (it != ubo_layout.members.end()) {
		//     memcpy(ubo_memory_ptr + it->offset, &current_time, it->size);
		// }

		// Example: Copying a vec3 'position' member
		// struct Vec3 { float x, y, z; };
		// Vec3 camera_pos = {10.0f, 5.0f, -3.0f};
		// auto it_pos = std::find_if(ubo_layout.members.begin(), ubo_layout.members.end(),
		//                            [](const UniformBufferMemberLayout& m){ return m.name ==
		//                            "position"; });
		// if (it_pos != ubo_layout.members.end()) {
		//     memcpy(ubo_memory_ptr + it_pos->offset, &camera_pos, it_pos->size);
		// }
		// Note: Be careful with padding and alignment for structs/arrays! spirv-reflect provides
		// the *exact* offset based on the SPIR-V layout rules (std140 or std430, typically std140
		// for UBOs). Ensure your C++ struct matches this layout or use the individual member
		// offsets.
	}

	PushConstantLayout pc_layout;
	success = getPushConstantLayout(
		spirv_blob.data(),
		spirv_blob.size() * sizeof(uint32_t), // size in bytes!
		pc_layout);

	if (success) {
		std::cout << "Successfully found push constant block:" << std::endl;
		std::cout << "  Block Name: " << pc_layout.block_name << std::endl;
		// std::cout << "  Stages: " << shaderStageFlagsToString(pc_layout.stage_flags) <<
		// std::endl;
		std::cout << "  Block Size (bytes): " << pc_layout.size << std::endl;
		std::cout << "  Members:" << std::endl;
		for (const auto& member : pc_layout.members) {
			std::cout << "    - Name: " << member.name << ", Offset: " << member.offset << " bytes"
					  << ", Size: " << member.size << " bytes" << std::endl;
		}

		// --- How you would use this for memcpy ---
		// Assume you have your push constant data in a C++ struct or array
		// struct PushConstantData {
		//     glm::vec4 color;
		//     float intensity;
		//     uint32_t id;
		// };
		// PushConstantData data = { {1.0f, 0.5f, 0.2f, 1.0f}, 2.5f, 123 };

		// You would typically copy the entire struct if its layout matches or
		// copy members individually if needed. Push constants are often copied
		// directly into command buffer space or a temporary buffer.
		//
		// uint8_t push_constant_buffer[pc_layout.size]; // Or use a vector

		// Example: Copying 'color' member
		// auto it_color = std::find_if(pc_layout.members.begin(), pc_layout.members.end(),
		//                             [](const BufferMemberLayout& m){ return m.name == "color";
		//                             });
		// if (it_color != pc_layout.members.end()) {
		//     memcpy(push_constant_buffer + it_color->offset, &data.color, it_color->size);
		// }

		// Example: Copying 'intensity' member
		// auto it_intensity = std::find_if(pc_layout.members.begin(), pc_layout.members.end(),
		//                                 [](const BufferMemberLayout& m){ return m.name ==
		//                                 "intensity"; });
		// if (it_intensity != pc_layout.members.end()) {
		//     memcpy(push_constant_buffer + it_intensity->offset, &data.intensity,
		//     it_intensity->size);
		// }
		// ... and so on for other members ...

		// Then update the push constants during command buffer recording
		// vkCmdPushConstants(... , pc_layout.stage_flags, 0, pc_layout.size, push_constant_buffer);
	}

	return spirv_blob;
}

gpu::ShaderObject gpu::compile_shaders(std::string vert_source, std::string frag_source)
{
	ShaderObject shader_object;

	const auto vertex_spirv_blob =
		glsl_to_sprv("vertex_shader", gpu::ShaderStage::VERTEX, vert_source, true);
	const auto fragment_spirv_blob =
		glsl_to_sprv("fragment_shader", gpu::ShaderStage::FRAGMENT, frag_source, true);

	{
		// Compute vertex layout
		std::vector<VertexAttributeLayout> vertex_layout;
		bool success = getVertexInputLayout(
			vertex_spirv_blob.data(),
			vertex_spirv_blob.size() * sizeof(uint32_t), // size in bytes!
			vertex_layout);

		// TODO(Seth) - this really should be using format enums
		for (const auto& attr : vertex_layout) {
			if (attr.size == 8) {
				shader_object.vertex_attributes.push_back(gpu::VertexAttribute::VEC2_FLOAT);
			}
			else if (attr.size == 12) {
				shader_object.vertex_attributes.push_back(gpu::VertexAttribute::VEC3_FLOAT);
			}
			else {
				// oops idk
			}
		}
	}

	if (strcmp(GPU_BACKEND, "Vulkan") == 0) {
		shader_object.target_vertex_shader = vertex_spirv_blob;
		shader_object.target_fragment_shader = fragment_spirv_blob;
	}
	else if (strcmp(GPU_BACKEND, "GL") == 0) {
		shader_object.target_vertex_shader = sprv_to_gles(vertex_spirv_blob);
		shader_object.target_fragment_shader = sprv_to_gles(fragment_spirv_blob);
	}
	else {
		std::cerr << "Unknown GPU backend: " << GPU_BACKEND << std::endl;
		return {};
	}

	return shader_object;
}
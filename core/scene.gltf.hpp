#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "scene.h"
#include "gpu.h"
#include "physics.h"
#include "tiny_gltf.h"
#include "scene.gltf.hpp"

#include <filesystem>
#include <iostream>
#include <memory>
#include <unordered_set>

/**
 * Docs with example:
 * https://github.com/syoyo/tinygltf/blob/release/examples/basic/main.cpp#L162
 */
void load_model_gltf(
	string_view model_path_raw, bool flip_uvs, bool flip_triangle_winding, gpu::RenderDevice* gpu)
{
	const string model_path = filesystem::current_path() / model_path_raw;

	tinygltf::TinyGLTF loader;
	tinygltf::Model gltf_model;
	string err, warn;

	bool res = loader.LoadBinaryFromFile(&gltf_model, &err, &warn, model_path);
	if (!warn.empty()) {
		cout << "Warning while parsing glTF model (" << model_path << "): " << warn << endl;
	}
	if (!err.empty()) {
		cout << "Error while parsing glTF model (" << model_path << "): " << err << endl;
	}
	if (!res) {
		cout << "Failed to load glTF: " << model_path << endl;
		return;
	}
	cout << "glTF model loaded from file: " << model_path << endl;
	cout << "glTF model has " << gltf_model.scenes.size() << " scenes" << endl;
	const tinygltf::Scene& gltf_scene = gltf_model.scenes[gltf_model.defaultScene];

	cout << "glTF has " << gltf_model.buffers.size() << " buffers." << endl;

	// Process model buffers
	const int TINYGLTF_UNKNOWN_TARGET = 0;
	const int TINYGLTF_VERTEX_BUFFER_TARGET = 34962;
	const int TINYGLTF_INDEX_BUFFER_TARGET = 34963;
	for (tinygltf::BufferView& gltf_buffer_view : gltf_model.bufferViews) {
		const tinygltf::Buffer& gltf_buffer = gltf_model.buffers[gltf_buffer_view.buffer];

		switch (gltf_buffer_view.target) {
		case TINYGLTF_UNKNOWN_TARGET:
			break;
		case TINYGLTF_VERTEX_BUFFER_TARGET: {
			// cout << "glTF decoded a vertex buffer: " << gltf_buffer.data.size() << ", "
			// 	 << gltf_buffer_view.byteOffset << " - " << gltf_buffer_view.byteLength << endl;
			// gpu::Buffer* vbo = gpu->create_buffer(
			// 	gpu::BufferUsage::VERTEX,
			// 	gltf_buffer_view.byteLength,
			// 	&gltf_buffer.data.at(0) + gltf_buffer_view.byteOffset);
			// gpu->destroy_buffer(vbo);
			break;
		}
		case TINYGLTF_INDEX_BUFFER_TARGET: {
			cout << "glTF decoded an index buffer: " << gltf_buffer.data.size() << ", "
				 << gltf_buffer_view.byteOffset << " - " << gltf_buffer_view.byteLength << endl;
			gpu::Buffer* vbo = gpu->create_buffer(
				gpu::BufferUsage::INDEX,
				gltf_buffer_view.byteLength,
				&gltf_buffer.data.at(0) + gltf_buffer_view.byteOffset);
			gpu->destroy_buffer(vbo);
			break;
		}
		default:
			cerr << "Error in glTF decoder: unknown value for tinygltf::BufferView::target "
				 << "\"" << gltf_buffer_view.target << "\"" << endl;
			return;
		}
	}

	// Process scene nodes
	for (size_t gltf_node_idx : gltf_scene.nodes) {
		assert((gltf_node_idx >= 0) && (gltf_node_idx < gltf_model.nodes.size()));
		const tinygltf::Node& gltf_node = gltf_model.nodes[gltf_node_idx];

		// If: this node has a mesh.
		if (gltf_node.mesh >= 0 && gltf_node.mesh < gltf_model.meshes.size()) {
			const tinygltf::Mesh& gltf_mesh = gltf_model.meshes[gltf_node.mesh];
			cout << gltf_node.name << ", " << gltf_mesh.name << endl;

			// Process each primitive in the mesh
			for (const tinygltf::Primitive& gltf_primitive : gltf_mesh.primitives) {

				cout << "Primitive:";
				const auto& gltf_index_accessor = gltf_model.accessors[gltf_primitive.indices];
				const auto& gltf_index_buffer_view =
					gltf_model.bufferViews[gltf_index_accessor.bufferView];
				const auto& gltf_index_buffer = gltf_model.buffers[gltf_index_buffer_view.buffer];
				gpu::Buffer* gpu_index_buffer = gpu->create_buffer(
					gpu::BufferUsage::INDEX,
					gltf_index_buffer_view.byteLength,
					&gltf_index_buffer.data.at(0) + gltf_index_buffer_view.byteOffset);
				gpu->destroy_buffer(gpu_index_buffer);

				const tinygltf::BufferView* gltf_position_buffer;
				const tinygltf::BufferView* gltf_normal_buffer;
				const tinygltf::BufferView* gltf_uv_buffer;

				for (const auto& [attrib_name, attrib_idx] : gltf_primitive.attributes) {
					const auto& attrib_accessor = gltf_model.accessors[attrib_idx];
					const auto& attrib_buffer_view = gltf_model.bufferViews[attrib_accessor.bufferView];
					if (attrib_name == "POSITION") gltf_position_buffer = &attrib_buffer_view;
					if (attrib_name == "NORMAL") gltf_normal_buffer = &attrib_buffer_view;
					if (attrib_name == "TEXCOORD_0") gltf_uv_buffer = &attrib_buffer_view;
				}
				cout << endl;
			}
		}
		// Else if: this node has no mesh.
		else {
			// Tell me.  I'm curious if this ever happens.
			// TODO(Seth) - delete after investigating
			cout << "glTF decoder found a node without a mesh." << endl;
		}
	}
}
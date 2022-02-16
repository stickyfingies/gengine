#include "assets.h"

#include "stb/stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <fstream>
#include <iostream>
#include <sstream>

namespace gengine {

auto load_image(std::string_view path) -> ImageAsset {
	auto width = 0;
	auto height = 0;
	auto channel_count = 0;

	const auto data =
		stbi_load(path.data(), &width, &height, &channel_count, 0);

	std::cout << "[info]\t loading image " << path.data() << std::endl;
	std::cout << "[info]\t\t size=" << width << "x" << height << std::endl;
	std::cout << "[info]\t\t channels=" << channel_count << std::endl;

	return {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
			static_cast<uint32_t>(channel_count), data};
}

auto unload_image(const ImageAsset &asset) -> void {
	stbi_image_free(asset.data);
}

auto load_vertex_buffer(std::string_view path)
	-> std::tuple<std::vector<float>, std::vector<unsigned int>> {
	static auto importer = Assimp::Importer{};

	std::cout << "[info]\t loading scene " << path.data() << std::endl;

	const auto scene = importer.ReadFile(path.data(), aiProcess_Triangulate |
														  aiProcess_GenNormals);

	// error handling

	if ((!scene) || (scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) ||
		(!scene->mRootNode)) {
		std::cerr << "[!ERR]\t\t unable to load scene!" << std::endl;
		return {};
	}

	auto vertices = std::vector<float>{};
	auto indices = std::vector<unsigned int>{};

	// accumulate geometry from all meshes in scene

	for (auto i = 0; i < scene->mNumMeshes; ++i) {
		const auto mesh = scene->mMeshes[i];

		std::cout << "[info]\t\t mesh " << i
				  << ": { vertices: " << mesh->mNumVertices
				  << ", faces: " << mesh->mNumFaces << " }" << std::endl;

		// accumulate vertices

		for (auto j = 0; j < mesh->mNumVertices; ++j) {
			vertices.push_back(mesh->mVertices[j].x);
			vertices.push_back(-mesh->mVertices[j].y);
			vertices.push_back(mesh->mVertices[j].z);

			vertices.push_back(mesh->mNormals[j].x);
			vertices.push_back(mesh->mNormals[j].y);
			vertices.push_back(mesh->mNormals[j].z);

			vertices.push_back(mesh->mTextureCoords[0][j].x);
			vertices.push_back(mesh->mTextureCoords[0][j].y);
		}

		// extract indices from faces

		for (auto j = 0; j < mesh->mNumFaces; ++j) {
			const auto face = mesh->mFaces[j];

			for (auto k = 0; k < face.mNumIndices; ++k) {
				indices.push_back(face.mIndices[k]);
			}
		}
	}

	return {vertices, indices};
}

auto load_file(std::string_view path) -> std::string {
	auto stream = std::ifstream(path.data(), std::ifstream::binary);

	auto buffer = std::stringstream{};

	buffer << stream.rdbuf();

	return buffer.str();
}

} // namespace gengine
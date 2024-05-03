#include "assets.h"

#include "stb/stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

namespace gengine {

auto load_image(std::string_view path) -> ImageAsset
{
	auto width = 0;
	auto height = 0;
	auto channel_count = 0;

	const auto data = stbi_load(path.data(), &width, &height, &channel_count, 0);

	std::cout << "[info]\t loading image " << path.data() << std::endl;
	std::cout << "[info]\t\t size=" << width << "x" << height << std::endl;
	std::cout << "[info]\t\t channels=" << channel_count << std::endl;

	return {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		static_cast<uint32_t>(channel_count),
		data};
}

auto unload_image(const ImageAsset& asset) -> void { stbi_image_free(asset.data); }

auto traverseNode(GeometryAssetList& assets, const aiScene* scene, const aiNode* node) -> void
{
	std::vector<aiNode*> parents{};
	aiNode* parent = node->mParent;
	while (parent != nullptr) {
		parents.push_back(parent);
		parent = parent->mParent;
	}
	auto worldTransform = glm::mat4{1.0f};
	for (int i = parents.size() - 1; i >= 0; i--) {
		const auto tr = glm::transpose(glm::make_mat4(&parents[i]->mTransformation.a1));
		worldTransform *= tr;
	}

	const auto transform =
		worldTransform; // glm::transpose(glm::make_mat4(&node->mTransformation.a1));

	for (auto i = 0; i < node->mNumMeshes; ++i) {
		const auto mesh_idx = node->mMeshes[i];
		const auto mesh = scene->mMeshes[mesh_idx];

		std::cout << "[info]\t\t mesh " << i << ": { vertices: " << mesh->mNumVertices
				  << ", faces: " << mesh->mNumFaces << " }" << std::endl;

		auto vertices = std::vector<float>{};
		auto vertices_aux = std::vector<float>{};
		auto indices = std::vector<unsigned int>{};

		// accumulate vertices

		for (auto j = 0; j < mesh->mNumVertices; ++j) {
			vertices.push_back(mesh->mVertices[j].x);
			vertices.push_back(mesh->mVertices[j].y);
			vertices.push_back(mesh->mVertices[j].z);

			vertices_aux.push_back(mesh->mNormals[j].x);
			vertices_aux.push_back(mesh->mNormals[j].y);
			vertices_aux.push_back(mesh->mNormals[j].z);

			vertices_aux.push_back(mesh->mTextureCoords[0][j].x);
			vertices_aux.push_back(mesh->mTextureCoords[0][j].y);
		}

		std::cout << "Vertices count: " << vertices.size() << std::endl;

		// extract indices from faces

		for (auto j = 0; j < mesh->mNumFaces; ++j) {
			const auto face = mesh->mFaces[j];
			for (auto k = 0; k < face.mNumIndices; ++k) {
				indices.push_back(face.mIndices[k]);
			}
		}

		// material stuff

		const auto loadTextures = [](std::vector<std::string>& texturePaths,
									 const aiMaterial* material,
									 aiTextureType type) -> void {
			for (uint32_t i = 0; i < material->GetTextureCount(type); i++) {
				aiString path;
				material->GetTexture(type, i, &path);
				texturePaths.push_back(path.C_Str());
			}
		};

		auto texturePaths = std::vector<std::string>{};

		if (mesh->mMaterialIndex >= 0) {
			const auto* material = scene->mMaterials[mesh->mMaterialIndex];
			loadTextures(texturePaths, material, aiTextureType_DIFFUSE);
		}

		for (const auto& path : texturePaths) {
			std::cout << "Texture: " << path << std::endl;
		}

		assets.push_back({transform, vertices, vertices_aux, indices, texturePaths});
	}

	for (auto i = 0; i < node->mNumChildren; ++i) {
		const auto child = node->mChildren[i];
		traverseNode(assets, scene, child);
	}
}

auto load_vertex_buffer(std::string_view path, bool flipUVs, bool flipWindingOrder)
	-> std::vector<GeometryAsset>
{
	// TODO - Fix ASSIMP flipping models upside down on import
	static auto importer = Assimp::Importer{};

	// load scene

	std::cout << "[info]\t loading scene " << path.data() << std::endl;

	uint32_t importFlags = aiProcess_Triangulate | aiProcess_GenNormals;
	if (flipUVs)
		importFlags |= aiProcess_FlipUVs;
	if (flipWindingOrder)
		importFlags |= aiProcess_FlipWindingOrder;
	const auto scene = importer.ReadFile(path.data(), importFlags);
	if ((!scene) || (scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) || (!scene->mRootNode)) {
		std::cerr << "[!ERR]\t\t unable to load scene!" << std::endl;
		return {};
	}

	// scene->mRootNode->mTransformation = aiMatrix4x4();

	auto geometry_assets = std::vector<GeometryAsset>();

	traverseNode(geometry_assets, scene, scene->mRootNode);

	return geometry_assets;
}

auto load_file(std::string_view path) -> std::string
{
	const auto stream = std::ifstream(path.data(), std::ifstream::binary);

	auto buffer = std::stringstream{};

	buffer << stream.rdbuf();

	return buffer.str();
}

} // namespace gengine
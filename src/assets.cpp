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
#include <unordered_map>

/// State belongs to the compilation unit
/// TODO make an AssetLoader class or something

static std::unordered_map<std::string, gengine::ImageAsset> image_cache;

namespace gengine {

auto get_loaded_images() -> ImageCache* { return &image_cache; }

auto load_image(std::string path) -> ImageAsset
{
	const std::string path_str{path.begin(), path.end()};

	if (image_cache.find(path_str) != image_cache.end()) {
		return image_cache.at(path_str);
	}

	auto width = 0;
	auto height = 0;
	auto channel_count = 0;

	const auto data = stbi_load(path.data(), &width, &height, &channel_count, 4);

	std::cout << "[info]\t loading image " << path.data() << std::endl;
	std::cout << "[info]\t\t size=" << width << "x" << height << std::endl;
	std::cout << "[info]\t\t channels=" << channel_count << " (fixed to 4)" << std::endl;

	const auto image_asset = ImageAsset{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		static_cast<uint32_t>(4),
		data,
		path_str};

	image_cache[path_str] = image_asset;

	return image_asset;
}

auto load_image_from_memory(std::string name, const unsigned char* buffer, uint32_t buffer_len)
	-> ImageAsset
{
	if (image_cache.find(name) != image_cache.end()) {
		return image_cache.at(name);
	}

	auto width = 0;
	auto height = 0;
	auto channel_count = 0;

	const auto data = stbi_load_from_memory(buffer, buffer_len, &width, &height, &channel_count, 4);

	std::cout << "[info]\t loading image from memory" << std::endl;
	std::cout << "[info]\t\t size=" << width << "x" << height << std::endl;
	std::cout << "[info]\t\t channels=" << channel_count << " (fixed to 4)" << std::endl;

	const auto image_asset = ImageAsset{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		static_cast<uint32_t>(4),
		data,
		name};

	image_cache[name] = image_asset;

	return image_asset;
}

auto unload_image(const ImageAsset& asset) -> void {
	image_cache.erase(asset.path);
	stbi_image_free(asset.data);
}

auto unload_all_images() -> void {
	for (auto it = image_cache.begin(); it != image_cache.end();) {
		stbi_image_free(it->second.data);
		image_cache.erase(it++);
	}
}

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

		const auto loadTextures = [scene](
									  std::vector<ImageAsset>& texturePaths,
									  const aiMaterial* material,
									  aiTextureType type) -> void {
			for (uint32_t i = 0; i < material->GetTextureCount(type); i++) {
				aiString path;
				material->GetTexture(type, i, &path);
				if (auto texture = scene->GetEmbeddedTexture(path.C_Str())) {
					// Embedded texture

					std::string path_string = path.C_Str();
					const auto index = std::atoi(path_string.substr(1).c_str());
					const auto embed = scene->mTextures[index];

					std::cout << embed->mWidth << " x " << embed->mHeight << std::endl;

					texturePaths.push_back(gengine::load_image_from_memory(
						path_string,
						reinterpret_cast<const unsigned char*>(embed->pcData),
						embed->mWidth));
				}
				else {
					// Regular texture
					texturePaths.push_back(gengine::load_image(path.C_Str()));
				}
			}
		};

		auto textures = std::vector<ImageAsset>{};
		aiColor3D material_color(1.f, 1.f, 1.f);

		if (mesh->mMaterialIndex >= 0) {
			const auto* material = scene->mMaterials[mesh->mMaterialIndex];
			material->Get(AI_MATKEY_COLOR_DIFFUSE, material_color);
			loadTextures(textures, material, aiTextureType_DIFFUSE);
		}

		assets.push_back(
			{transform,
			 vertices,
			 vertices_aux,
			 indices,
			 textures,
			 glm::vec3{material_color.r, material_color.g, material_color.b}});
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
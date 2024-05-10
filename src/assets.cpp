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

namespace gengine {

/// An append-only log of all the images from this loader
static ImageLog image_log;

/// A CRUD cache of images persisted in system memory
static ImageCache image_cache;

auto get_image_log() -> const ImageLog* { return &image_log; }

auto get_image_cache() -> const ImageCache* { return &image_cache; }

auto image_in_cache(const std::string& path) -> bool
{
	return image_cache.find(path) != image_cache.end();
}

auto load_image_from_file(const std::string& path) -> std::expected<ImageAsset, std::string>
{
	// Return the cached asset
	if (image_in_cache(path)) {
		return image_cache.at(path);
	}

	auto width = 0;
	auto height = 0;
	auto channel_count = 0;

	const auto data = stbi_load(path.data(), &width, &height, &channel_count, 4);

	if (data == nullptr) {
		return std::unexpected("Cannot load " + path);
	}

	const auto image_asset = ImageAsset{
		path,
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		static_cast<uint32_t>(4),
		data};

	image_log.push_back(image_asset);
	std::cout << "[info]\t ImageAsset " << path.data() << " (" << width << "x" << height
			  << ") channels=" << channel_count << " (fixed to 4)" << std::endl;

	image_cache[path] = image_asset;

	return image_asset;
}

auto load_image_from_memory(
	const std::string& name, const unsigned char* buffer, uint32_t buffer_len) -> ImageAsset
{
	if (image_cache.find(name) != image_cache.end()) {
		return image_cache.at(name);
	}

	auto width = 0;
	auto height = 0;
	auto channel_count = 0;

	const auto data = stbi_load_from_memory(buffer, buffer_len, &width, &height, &channel_count, 4);

	const auto image_asset = ImageAsset{
		name,
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		static_cast<uint32_t>(4),
		data};

	image_log.push_back(image_asset);
	std::cout << "[info]\t ImageAsset " << name << " (" << width << "x" << height
			  << ") channels=" << channel_count << " (fixed to 4)" << std::endl;

	image_cache[name] = image_asset;

	return image_asset;
}

auto unload_image(const ImageAsset& asset) -> void
{
	image_cache.erase(asset.name);
	stbi_image_free(asset.data);
}

auto unload_all_images() -> void
{
	for (auto it = image_cache.begin(); it != image_cache.end();) {
		std::cout << "[info]\t ~ ImageAsset " << it->first << std::endl;
		stbi_image_free(it->second.data);
		image_cache.erase(it++);
	}
}

auto traverseNode(MeshAssetList& assets, const aiScene* scene, const aiNode* node) -> void
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

		std::cout << "[info]\t Mesh " << i << " { vertices: " << mesh->mNumVertices
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

		// extract indices from faces

		for (auto j = 0; j < mesh->mNumFaces; ++j) {
			const auto face = mesh->mFaces[j];
			for (auto k = 0; k < face.mNumIndices; ++k) {
				indices.push_back(face.mIndices[k]);
			}
		}

		// material stuff

		const auto loadTextures = [&scene](
									  std::vector<ImageAsset>& texturePaths,
									  const aiMaterial* material,
									  aiTextureType type) -> void {
			for (uint32_t i = 0; i < material->GetTextureCount(type); i++) {
				// Get the path of this texture
				aiString path;
				material->GetTexture(type, i, &path);

				// Embedded texture
				if (auto texture = scene->GetEmbeddedTexture(path.C_Str())) {

					// Grab the embedded texture instance
					const std::string path_string = path.C_Str();
					const auto index = std::atoi(path_string.substr(1).c_str());
					const auto embed = scene->mTextures[index];

					// Load from memory
					const auto imageAsset = load_image_from_memory(
						path_string,
						reinterpret_cast<const unsigned char*>(embed->pcData),
						embed->mWidth);

					texturePaths.push_back(imageAsset);
				}
				// Regular texture (load from file)
				else if (const auto data = load_image_from_file(path.C_Str()); data.has_value()) {
					texturePaths.push_back(*data);
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

/// TODO - make this return 'expected<MeshAsset, AssetError>'
auto load_model(std::string_view path, bool flipUVs, bool flipWindingOrder)
	-> std::vector<MeshAsset>
{
	static auto importer = Assimp::Importer{};

	std::cout << "[info]\t Scene " << path.data() << std::endl;

	uint32_t importFlags = aiProcess_Triangulate | aiProcess_GenNormals;
	if (flipUVs) {
		importFlags |= aiProcess_FlipUVs;
	}
	if (flipWindingOrder) {
		importFlags |= aiProcess_FlipWindingOrder;
	}

	const auto scene = importer.ReadFile(path.data(), importFlags);
	if ((!scene) || (scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) || (!scene->mRootNode)) {
		std::cerr << "[!ERR]\t\t unable to load scene!" << std::endl;
		return {};
	}

	auto geometry_assets = std::vector<MeshAsset>();

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
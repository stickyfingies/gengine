#include "assets.h"
#include "stb/stb_image.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/fetch.h>
#endif

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/quaternion.h>
#include <assimp/scene.h>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

/// State belongs to the compilation unit
/// TODO make an AssetLoader class or something

using namespace std;

namespace gengine {

auto TextureFactory::get_image_log() -> const ImageLog* { return &image_log; }

auto TextureFactory::get_image_cache() -> const ImageCache* { return &image_cache; }

auto TextureFactory::image_in_cache(const std::string& path) -> bool
{
	return image_cache.find(path) != image_cache.end();
}

auto TextureFactory::load_image_from_file(const std::string& path)
	-> std::expected<ImageAsset, std::string>
{
	// Return the cached asset
	if (image_in_cache(path)) {
		return image_cache.at(path);
	}

	auto width = 0;
	auto height = 0;
	auto channel_count = 0;

	filesystem::path normalized_path = filesystem::current_path() / path;

	const auto data = stbi_load(normalized_path.c_str(), &width, &height, &channel_count, 4);

	if (data == nullptr) {
		cout << "Error: Cannot load " << normalized_path.string() << endl;
		// abort();
		return std::unexpected("Cannot load " + normalized_path.string());
	}

	const auto image_asset = ImageAsset{
		normalized_path,
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		static_cast<uint32_t>(4),
		data};

	image_log.push_back(image_asset);
	cout << "ImageAsset " << path.data() << " (" << width << "x" << height
		 << ") channels=" << channel_count << " (fixed to 4)" << endl;

	image_cache[path] = image_asset;

	return image_asset;
}

auto TextureFactory::load_image_from_memory(
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
	cout << "ImageAsset " << name << " (" << width << "x" << height
		 << ") channels=" << channel_count << " (fixed to 4)" << endl;

	image_cache[name] = image_asset;

	return image_asset;
}

auto TextureFactory::unload_image(const ImageAsset& asset) -> void
{
	image_cache.erase(asset.name);
	stbi_image_free(asset.data);
}

auto TextureFactory::unload_all_images() -> void
{
	for (auto it = image_cache.begin(); it != image_cache.end();) {
		cout << "~ ImageAsset " << it->first << endl;
		stbi_image_free(it->second.data);
		image_cache.erase(it++);
	}
}

/// @brief Temporary structure used to track entity relationships
///        while decoding an Assimp scene.
struct AssetDecoding {
	using EmbedIdx = size_t;
	using ObjectIdx = size_t;
	using MeshIdx = size_t;
	using MaterialIdx = size_t;

	size_t objectCount = 0;

	/// Assimp texture --> Objects
	std::unordered_map<EmbedIdx, std::vector<MaterialIdx>> embeds_to_materials;

	/// Assimp texture --> Objects
	std::unordered_map<std::string, std::vector<MaterialIdx>> texpaths_to_materials;

	/// Assimp mesh --> Objects
	std::unordered_map<MeshIdx, std::vector<ObjectIdx>> mesh_to_objects;

	/// Assimp material --> Objects
	std::unordered_map<MaterialIdx, std::vector<ObjectIdx>> material_to_objects;
};

auto calculateWorldTransform(const aiNode* node) -> glm::mat4
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
	return worldTransform;
};

auto traverseNode(
	AssetDecoding& decoding, SceneAsset& assets, const aiScene* scene, const aiNode* node) -> void
{
	const auto transform = calculateWorldTransform(node);

	for (auto i = 0; i < node->mNumMeshes; ++i) {
		const auto mesh_idx = node->mMeshes[i];
		const auto mesh = scene->mMeshes[mesh_idx];
		decoding.mesh_to_objects[mesh_idx].push_back(decoding.objectCount);
		decoding.objectCount += 1;
		assets.objects.push_back({transform, 0, 0});
	}

	for (auto i = 0; i < node->mNumChildren; ++i) {
		const auto child = node->mChildren[i];
		traverseNode(decoding, assets, scene, child);
	}
}

auto processGeometry(const aiScene* scene, size_t mesh_idx, size_t& material_idx) -> GeometryAsset
{
	const auto ai_mesh = scene->mMeshes[mesh_idx];

	const size_t vertex_buffer_size = ai_mesh->mNumVertices * 3 * sizeof(float);
	const size_t normal_buffer_size = ai_mesh->mNumVertices * 3 * sizeof(float);
	const size_t uv_buffer_size = ai_mesh->mNumVertices * 2 * sizeof(float);
	const size_t index_buffer_size = ai_mesh->mNumFaces * 3 * sizeof(unsigned int);
	const size_t mesh_size =
		vertex_buffer_size + normal_buffer_size + uv_buffer_size + index_buffer_size;

	std::cout << "Mesh " << mesh_idx << " (" << mesh_size << " bytes)" << std::endl;

	auto vertices = std::vector<float>(ai_mesh->mNumVertices * 3);
	auto vertices_aux = std::vector<float>(ai_mesh->mNumVertices * 5);
	auto indices = std::vector<unsigned int>(ai_mesh->mNumFaces * 3);

	// accumulate vertices

	for (auto j = 0; j < ai_mesh->mNumVertices; ++j) {
		vertices[j * 3 + 0] = ai_mesh->mVertices[j].x;
		vertices[j * 3 + 1] = ai_mesh->mVertices[j].y;
		vertices[j * 3 + 2] = ai_mesh->mVertices[j].z;

		vertices_aux[j * 5 + 0] = ai_mesh->mNormals[j].x;
		vertices_aux[j * 5 + 1] = ai_mesh->mNormals[j].y;
		vertices_aux[j * 5 + 2] = ai_mesh->mNormals[j].z;

		vertices_aux[j * 5 + 3] = ai_mesh->mTextureCoords[0][j].x;
		vertices_aux[j * 5 + 4] = ai_mesh->mTextureCoords[0][j].y;
	}

	// extract indices from faces

	for (auto j = 0; j < ai_mesh->mNumFaces; ++j) {
		const auto face = ai_mesh->mFaces[j];
		for (auto k = 0; k < face.mNumIndices; ++k) {
			indices[j * 3 + k] = face.mIndices[k];
		}
	}

	material_idx = ai_mesh->mMaterialIndex;

	return {vertices, vertices_aux, indices};
}

auto extractTextures(
	const aiScene* scene,
	AssetDecoding& decoding,
	const aiMaterial* material,
	uint32_t material_idx,
	aiTextureType type) -> void
{
	for (uint32_t i = 0; i < material->GetTextureCount(type); i++) {
		// Get the path of this texture
		aiString path;
		material->GetTexture(type, i, &path);
		const std::string path_string = string(path.C_Str());

		// Embedded texture
		if (auto texture = scene->GetEmbeddedTexture(path.C_Str())) {
			// Grab the embedded texture instance
			const auto index = std::atoi(path_string.substr(1).c_str());

			decoding.embeds_to_materials[index].push_back(material_idx);
		}
		// Regular texture (load from file)
		else {
			decoding.texpaths_to_materials[path_string].push_back(material_idx);
		}
	}
};

/// TODO - make this return 'expected<MeshAsset, AssetError>'
auto load_model(
	TextureFactory& texture_factory,
	std::string_view path,
	bool flipUVs,
	bool flipWindingOrder) -> SceneAsset
{
	static auto importer = Assimp::Importer{};

	filesystem::path normalized_path = filesystem::current_path() / path;

	cout << "Scene path: " << normalized_path << endl;

	uint32_t importFlags = aiProcess_Triangulate | aiProcess_GenNormals;
	if (flipUVs) {
		importFlags |= aiProcess_FlipUVs;
	}
	if (flipWindingOrder) {
		importFlags |= aiProcess_FlipWindingOrder;
	}

	// ifstream file(normalized_path.c_str(), ios::binary | ios::ate);

	// if (!file.is_open()) {
	// 	cout << "Error: cannot locate model asset file " << normalized_path.c_str() << endl;
	// }
	// size_t fileSize = file.tellg();
	// file.seekg(0);
	// char* pBuffer = new char[fileSize];
	// file.read(pBuffer, fileSize);
	// file.close();
	// cout << "Scene " << normalized_path.c_str() << " with size " << fileSize << endl;

	// const auto scene = importer.ReadFileFromMemory(pBuffer, fileSize, importFlags, "GLFW2");
	const auto ai_scene = importer.ReadFile(normalized_path.c_str(), importFlags);
	if ((!ai_scene) || (ai_scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) || (!ai_scene->mRootNode)) {
		std::cerr << "Error: Scene cannot be located: " << normalized_path << std::endl;
		// delete[] pBuffer;
		// abort();
		return {};
	}

	for (int ai_mesh_idx = 0; ai_mesh_idx < ai_scene->mNumMeshes; ai_mesh_idx++) {
		const aiMesh* ai_mesh = ai_scene->mMeshes[ai_mesh_idx];
		const size_t vertex_buffer_size = ai_mesh->mNumVertices * 3 * sizeof(float);
		const size_t normal_buffer_size = ai_mesh->mNumVertices * 3 * sizeof(float);
		const size_t uv_buffer_size = ai_mesh->mNumVertices * 2 * sizeof(float);
		const size_t index_buffer_size = ai_mesh->mNumFaces * 3 * sizeof(unsigned int);
		const size_t mesh_size =
			vertex_buffer_size + normal_buffer_size + uv_buffer_size + index_buffer_size;
	}

	// delete[] pBuffer;

	auto decoding = AssetDecoding{};
	auto assets = SceneAsset{};

	traverseNode(decoding, assets, ai_scene, ai_scene->mRootNode);

	// Load meshes
	for (const auto& [mesh_idx, object_indices] : decoding.mesh_to_objects) {
		size_t material_idx;
		const auto geometry = processGeometry(ai_scene, mesh_idx, material_idx);

		for (const auto object_idx : object_indices) {
			assets.objects[object_idx].geometry = assets.geometries.size();

			if (material_idx >= 0) {
				decoding.material_to_objects[material_idx].push_back(object_idx);
			}
		}

		assets.geometries.push_back(geometry);
	}

	// Load materials
	for (const auto& [material_idx, object_indices] : decoding.material_to_objects) {
		const auto* material = ai_scene->mMaterials[material_idx];

		aiColor3D material_color(1.f, 1.f, 1.f);
		material->Get(AI_MATKEY_COLOR_DIFFUSE, material_color);
		const auto color = glm::vec3{material_color.r, material_color.g, material_color.b};

		extractTextures(ai_scene, decoding, material, material_idx, aiTextureType_DIFFUSE);

		for (const auto object_idx : object_indices) {
			assets.objects[object_idx].material = assets.materials.size();
		}

		assets.materials.push_back(gengine::MaterialAsset({}, color));
	}

	// Load textures from disk
	for (const auto& [texture_path, material_indices] : decoding.texpaths_to_materials) {
		const auto imageAsset = texture_factory.load_image_from_file(texture_path);
		if (imageAsset.has_value()) {
			// Assign texture to all meshes which use it
			for (auto material_idx : material_indices) {
				// This is because .OBJ material indexing begins at 1, not 0
				if (material_idx == assets.materials.size()) {
					material_idx -= 1;
				}
				assets.materials[material_idx].textures.push_back(*imageAsset);
			}
		}
	}

	// Load textures from memory
	for (const auto& [embed_idx, material_indices] : decoding.embeds_to_materials) {
		const auto embed = ai_scene->mTextures[embed_idx];
		const auto texture_name = std::to_string(embed_idx);

		// Load from memory
		const auto imageAsset = texture_factory.load_image_from_memory(
			texture_name, reinterpret_cast<const unsigned char*>(embed->pcData), embed->mWidth);

		// Assign texture to all meshes which use it
		for (const auto material_idx : material_indices) {
			assets.materials[material_idx].textures.push_back(imageAsset);
		}
	}

	return assets;
}

auto load_file(std::string_view path) -> std::string // TODO? return std::optional<std::string>
{
#ifdef FALSE
	cout << "Loading " << path.data() << endl;
	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	cout << "Before strcpy" << endl;
	strcpy(attr.requestMethod, "GET");
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS;
	cout << "Before fetch" << endl;
	emscripten_fetch_t* fetch = emscripten_fetch(
		&attr, "./data/compile.sh"); // Blocks here until the operation is complete.
	if (fetch->status == 200) {
		printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
		// The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
	}
	else {
		printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
	}
	emscripten_fetch_close(fetch);
	return "";
#else
	filesystem::path normalized_path = filesystem::current_path() / path;
	const auto stream = ifstream(normalized_path.c_str(), ifstream::binary);

	if (!stream) {
		cout << "Error: failed to open file " << normalized_path << endl;
		// abort();
		return ""; // TODO: see function signature
	}

	cout << "File path: " << normalized_path << endl;

	stringstream buffer{};
	buffer << stream.rdbuf();
	return buffer.str();
#endif
}

} // namespace gengine
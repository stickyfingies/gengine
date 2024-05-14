#pragma once

#include <glm/glm.hpp>

#include <expected>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <memory>

namespace gengine {

template <typename D> struct GenericImageAsset {

	

	std::string name;
	unsigned int width;
	unsigned int height;
	unsigned int channel_count;
	D* data;
};

/// Lives in RAM.
using ImageAsset = GenericImageAsset<unsigned char>;

class TextureFactory {
public:
	using ImageLog = std::vector<ImageAsset>;

	using ImageCache = std::unordered_map<std::string, ImageAsset>;

	auto load_image_from_file(const std::string& path) -> std::expected<ImageAsset, std::string>;

	auto load_image_from_memory(
		const std::string& name, const unsigned char* buffer, uint32_t buffer_len) -> ImageAsset;

	auto unload_image(const ImageAsset& asset) -> void;

	auto unload_all_images() -> void;

	auto get_image_log() -> const ImageLog*;

	auto get_image_cache() -> const ImageCache*;

private:
	/// An append-only log of all the images from this loader
	ImageLog image_log;

	/// A CRUD cache of images persisted in system memory
	ImageCache image_cache;

	auto image_in_cache(const std::string& path) -> bool;
};

struct GeometryAsset {
	std::vector<float> vertices;	 // raw positions
	std::vector<float> vertices_aux; // normals, uvs
	std::vector<unsigned int> indices;
};

struct MaterialAsset {
	std::vector<ImageAsset> textures;
	glm::vec3 color;
};

/// @brief A mesh belongs to a scene.
struct MeshAsset {
	glm::mat4 transform;
	/// Index into SceneAsset::geometries
	size_t geometry;
	/// Index into SceneAsset::materials
	size_t material;
};

struct SceneAsset {
	std::vector<MeshAsset> objects;
	std::vector<GeometryAsset> geometries;
	std::vector<MaterialAsset> materials;
};

auto load_model(TextureFactory& texture_factory, std::string_view path, bool flipUVs = false, bool flipWindingOrder = false)
	-> SceneAsset;

auto load_file(std::string_view path) -> std::string;
} // namespace gengine
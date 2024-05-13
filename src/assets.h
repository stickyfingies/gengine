#pragma once

#include <glm/glm.hpp>

#include <expected>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

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

struct GeometryAsset {
	std::vector<float> vertices;	 // raw positions
	std::vector<float> vertices_aux; // normals, uvs
	std::vector<unsigned int> indices;
};

struct MaterialAsset {
	std::vector<ImageAsset> textures;
	glm::vec3 color;
};

struct SceneAsset {

	using ObjectIdx = size_t;
	using GeometryIdx = size_t;
	using MaterialIdx = size_t;

	std::vector<glm::mat4> object_transforms;
	std::vector<GeometryAsset> geometries;
	std::vector<MaterialAsset> materials;

	std::unordered_map<ObjectIdx, GeometryIdx> object_to_geometry;
	std::unordered_map<ObjectIdx, MaterialIdx> object_to_material;
};

struct MeshAsset {
	glm::mat4 transform;
	GeometryAsset geometry;
	MaterialAsset material;
};

using MeshAssetList = std::vector<MeshAsset>;

using ImageLog = std::vector<ImageAsset>;

using ImageCache = std::unordered_map<std::string, ImageAsset>;

auto load_image_from_file(const std::string& path) -> std::expected<ImageAsset, std::string>;

auto unload_image(const ImageAsset& asset) -> void;

auto unload_all_images() -> void;

auto get_image_log() -> const ImageLog*;

auto get_image_cache() -> const ImageCache*;

auto load_model(std::string_view path, bool flipUVs = false, bool flipWindingOrder = false)
	-> MeshAssetList;

auto load_file(std::string_view path) -> std::string;
} // namespace gengine
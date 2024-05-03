#pragma once

#include <glm/glm.hpp>

#include <string_view>
#include <tuple>
#include <vector>

namespace gengine {

struct GeometryAsset {
	glm::mat4 transform;
	std::vector<float> vertices; // raw positions
	std::vector<float> vertices_aux; // normals, uvs
	std::vector<unsigned int> indices;
	std::vector<std::string> texturePaths;
};

using GeometryAssetList = std::vector<GeometryAsset>;

struct ImageAsset {
	unsigned int width;
	unsigned int height;
	unsigned int channel_count;
	unsigned char* data;
};

auto load_image(std::string_view path) -> ImageAsset;

auto unload_image(const ImageAsset& asset) -> void;

auto load_vertex_buffer(std::string_view path, bool flipUVs = false, bool flipWindingOrder = false) -> GeometryAssetList;

auto load_file(std::string_view path) -> std::string;
} // namespace gengine
#pragma once

#include <glm/glm.hpp>

#include <string_view>
#include <tuple>
#include <vector>

namespace gengine {

struct GeometryAsset {
	glm::mat4 transform;
	std::vector<float> vertices;
	std::vector<unsigned int> indices;
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

auto load_vertex_buffer(std::string_view path) -> GeometryAssetList;

auto load_file(std::string_view path) -> std::string;
} // namespace gengine
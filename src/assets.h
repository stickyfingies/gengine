#pragma once

#include <string_view>
#include <tuple>
#include <vector>

namespace gengine {
struct ImageAsset {
	unsigned int width;
	unsigned int height;
	unsigned int channel_count;
	unsigned char *data;
};

auto load_image(std::string_view path) -> ImageAsset;

auto unload_image(const ImageAsset &asset) -> void;

auto load_vertex_buffer(std::string_view path)
	-> std::tuple<std::vector<float>, std::vector<unsigned int>>;

auto load_file(std::string_view path) -> std::string;
} // namespace gengine
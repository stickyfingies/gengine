#pragma once

#include <tuple>
#include <string_view>
#include <vector>

namespace gengine
{
    auto load_vertex_buffer(std::string_view path) -> std::tuple<std::vector<float>, std::vector<unsigned int>>;
}
#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <memory>

// For use with GPU
template <typename T> using ptr = std::unique_ptr<T, void (*)(T*)>;
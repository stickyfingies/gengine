#pragma once

// Platform detection
#ifdef __EMSCRIPTEN__
    #define GENGINE_PLATFORM_WEB 1
#else
    #define GENGINE_PLATFORM_WEB 0
#endif

#if defined(__linux__) || defined(__linux) || defined(linux)
    #define GENGINE_PLATFORM_LINUX 1
#else
    #define GENGINE_PLATFORM_LINUX 0
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define GENGINE_PLATFORM_WINDOWS 1
#else
    #define GENGINE_PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__)
    #define GENGINE_PLATFORM_APPLE 1
#else
    #define GENGINE_PLATFORM_APPLE 0
#endif

// Common platform-specific includes
#if GENGINE_PLATFORM_WEB
    #include <emscripten.h>
#endif
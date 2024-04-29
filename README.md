# gengine
![Vulkan](https://a11ybadges.com/badge?logo=vulkan)
![C++](https://a11ybadges.com/badge?logo=cplusplus)
![CMake](https://a11ybadges.com/badge?logo=cmake)

A hobby game engine (ish-thing) I'm writing on the side.  I've actually written a game, called The Grove, that runs on my own engine written in Typescript.  One of my ultimate goals is to port all of the game-specific scripts for TG over from TypeScript -> WebAssembly, and then have the game run natively on this game engine, as well.

## Building

Download the dependencies...
```sh
$ git clone https://github.com/microsoft/vcpkg
$ ./vcpkg/bootstrap-vcpkg.sh
$ ./vcpkg/vcpkg install glfw3 bullet3 assimp
```

...Then build the project.
```sh
$ cmake -B build -S .
$ cmake --build build
```

## Resources
- [Interleaved or Separate Vertex Buffers](https://www.reddit.com/r/vulkan/comments/rtpdvu/interleaved_vs_separate_vertex_buffers/)

## Todo
- [x] switch from Renderer to RenderDevice
- [ ] Un-leave vertex positions from other buffer data
- [ ] unify command buffers on both sides of API wall (?)
- [ ] Remove that **atrocious** `MODULE_CALLBACK` garbage (what was I thinking??)
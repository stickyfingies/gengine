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

## Todo
- [x] switch from Renderer to RenderDevice
- [ ] unify command buffers on both sides of API wall (?)
- [ ] Remove that **atrocious** `MODULE_CALLBACK` garbage (what was I thinking??)
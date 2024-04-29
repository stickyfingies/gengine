# GameEngine

![PyBullet](https://a11ybadges.com/badge?logo=code&text=PyBullet&badgeColor=orange)
![Vulkan](https://a11ybadges.com/badge?logo=vulkan)
![C++](https://a11ybadges.com/badge?logo=cplusplus)
![CMake](https://a11ybadges.com/badge?logo=cmake)

A hobby game engine (ish-thing) I'm writing on the side.  I've actually written a game, called [The Grove](https://github.com/stickyfingies/grove/), that runs on my own engine written in Typescript.  One of my ultimate goals is to port all of the game-specific scripts for TG over from TypeScript -> WebAssembly, and then have the game run natively on this game engine, as well.

![Screenshot](./screenshot.png "Screenshot")

Getting Started
---
Download the project using git.
```sh
git clone https://github.com/stickyfingies/gengine.git
```
Activate and run the [build script](./build.sh) to create an executable.
```sh
chmod +x ./build.sh
./build.sh
```
- **Tip!** Append `-i` or `--install` to re-install dependencies.
- **Tip!** Append `-w` or `--watch` to enable watch mode for live re-compilation.

Finally, launch the app!
```sh
./dist/bin/gengine # good job, you're done!
```

## Resources
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Interleaved or Separate Vertex Buffers](https://www.reddit.com/r/vulkan/comments/rtpdvu/interleaved_vs_separate_vertex_buffers/)

## Todo
- [x] switch from Renderer to RenderDevice
- [ ] Un-leave vertex positions from other buffer data
- [ ] unify command buffers on both sides of API wall (?)
- [ ] Remove that **atrocious** `MODULE_CALLBACK` garbage (what was I thinking??)
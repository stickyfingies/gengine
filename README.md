# GameEngine

![Bullet](https://a11ybadges.com/badge?text=Bullet&badgeColor=goldenrod&logo=crosshair)
![Vulkan](https://a11ybadges.com/badge?logo=vulkan)
![C++](https://a11ybadges.com/badge?logo=cplusplus)
![CMake](https://a11ybadges.com/badge?logo=cmake)

A hobby real-time 3D simulation with physics, user input, and graphics.

![Screenshot](./screenshot.png "Screenshot")

A minimal editor + debug menu is also included.

![Screenshot](./screenshot_editor.png "Screenshot")

Getting Started
---

Download the project using git.

```sh
git clone https://github.com/stickyfingies/gengine.git
chmod +x ./build.sh
```

Build the project to create an executable.

```sh
./build.sh
```

Finally, launch the app!

```sh
./dist/bin/gengine
```

#### Build Script Options

- `-i` or `--install` will forcefully re-install dependencies.
- `-w` or `--watch` automatigally compiles C++ files when they change.

> [!NOTE]  
> The [github repo](https://github.com/stickyfingies/gengine) is equipped with Github Actions CI that automatically builds the project when you push to `master`.

#### WebAssembly support

Install `emscripten` and export the variable `$EMSCRIPTEN_ROOT` as the Emscripten SDK path.

Architecture
---

```mermaid
---
title: Loading Objects from Asset Files
---
flowchart TB

    Asset --> Matrix & Geometry & Texture

    Geometry --> Collidable & Renderable

    Texture --> Descriptor[Material]

    Matrix --> Collidable

    Collidable & Renderable & Descriptor --> GameObject
```

Gengine uses the [Asset Importer Library](https://assimp.org/) to pull geometry and texture data from static asset files.

The geometry is passed into the physics engine to create a physically simulable representation of that shape.

The geometry and the texture are passed into the rendering engine to create a structure that can be rendered on the GPU.

Finally, the culmination of these are used to create a cohesive "game object" that is both visible and tangible.

## Resources
- [Learn OpenGL](https://learnopengl.com/) <small>**Start here** — this tutorial taught me C++. It's that good.</small>
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Guide - Resources](https://vkguide.dev/docs/great_resources)
- [Writing an efficient Vulkan renderer](https://zeux.io/2020/02/27/writing-an-efficient-vulkan-renderer/)
- [Interleaved or Separate Vertex Buffers](https://www.reddit.com/r/vulkan/comments/rtpdvu/interleaved_vs_separate_vertex_buffers/)

Dependencies
---

All software dependencies are installed and managed by [vcpkg](https://vcpkg.io/).

- [glfw3](https://www.glfw.org/): cross-platform window creation and input
- [bullet3](https://pybullet.org/wordpress/): physics simulation for video games
- [assimp](http://assimp.org/): load and parse various 3d file formats
- [glm](https://github.com/g-truc/glm): mathematics library for graphics software
- [stb](https://github.com/nothings/stb): image loading & decoding from files and memory
- [imgui](https://github.com/ocornut/imgui): bloat-free graphical user interface for C++

Roadmap
---

- [x] Resizable windows
- [x] Colors and Textures
- [x] Texture mip-mapping
- [x] Integrate a GUI
- [ ] Lighting
- [ ] Edit, save, and load scenes
- [ ] Benchmarks and performance stats

License
---
Copyright (c) 2019 Seth Traman.

GPLv3.  See [COPYING](./COPYING).
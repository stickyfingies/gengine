# Running Examples

---

::: tip
Make sure you've finshed [setup](/setup) before running examples.
:::

---

In the `examples/` directory, each `.js` and `.cpp` file represents one entire application.  This section will focus on C++ because JavaScript isn't fully supported yet.

I encourage you to follow along with the instructions so you can try it for yourself!

### Choosing a Platform

Examples are built by constructing a **platform string** like `platform`-`gpu`-`releaseType`.

- Supported Platforms are `linux` and `web`.
- Supported GPU modules are `vk` and `gl`.
- Supported release types are `dev` and `app`.

### Developing for Desktop

For example, to build all the examples for desktop Linux using the Vulkan renderer with debug symbols included, we can run:

```sh
# writes to 'artifacts/linux-vk-dev'
cmake --workflow --preset linux-vk-dev
```

And to run the `native.cpp` example:

```sh
./artifacts/linux-vk-dev/examples/native/native.bin
```

### Publishing for Desktop

Creating a distributable for your video game is a very similar process to what we just did above.

```sh
# writes to artifacts/linux-vk-app
# creates .zip in artifacts/
cmake --workflow --preset linux-vk-app
```

By using the `app` release type, a .zip of your game, including the executables and all game assets, will be placed in the `artifacts/` folder.

### Developing for web

This process is identical to developing for desktop.

::: warning
Web platforms cannot use the `vk` GPU implementation, as it is designed only for desktops.
:::

```sh
# writes to 'artifacts/web-gl-dev'
cmake --workflow --preset web-gl-dev
```

This will create an HTML+JS+WASM bundle in the directory `artifacts/web-gl-dev`.

We can see the fruits of our labor by opening a web server inside that directory:

```sh
cd artifacts/web-gl-dev/examples
python3 -m http.server
# navigate to the displayed web page URL
```

### Publishing for web

Same as before.

```sh
cmake --workflow --preset web-gl-app
```
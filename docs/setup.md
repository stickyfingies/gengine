# Setup

Thank you for wanting to try out Game Engine!

### Git your Tools

The following tools must exist on your system:
- [cmake](https://cmake.org/download/)
- [git](https://git-scm.com/downloads)

::: info
Game Engine is not tested with `clang` and we recommend you use the `gcc` compiler.
:::

### Clone the Repository

```sh
git clone https://github.com/stickyfingies/gengine.git
```

The following lines are also necessary, because this project uses `git` submodules:


```sh
git submodule init
git submodule update
```

### 10 minute break

To guarantee consistent results across platforms, the engine builds its dependencies from source.  The amount of time this takes may vary, depending on your computer specs and internet speed.

Run `setup.sh` and grab a coffee while it builds :)

```sh
chmod +x setup.sh
./setup.sh
```

And you're done!
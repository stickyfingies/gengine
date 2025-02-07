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

### Running Examples

The `examples/` directory contains C++ and JavaScript game scripts.  Each example script can be built into an entire executable game using VSCode.  To understand how to build the examples without VSCode, see the [build guide](/build).

Open the `examples/` directory and find a C++ file.  With the C++ file open, locate the debugger panel on VSCode.

![Screenshot of VSCode's debugger panel](/vscode-debug-examples.png)

:::info
The "run" button is meant for examples, and **will fail** if your currently open file is not in the `examples/` directory.
:::

Click "run" and happy gaming!
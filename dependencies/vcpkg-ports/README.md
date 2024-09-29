## Tutorial

This tutorial is made for existing projects that use vcpkg to manage their C++ dependencies.

Sometimes, it becomes necessary to modify some elements of a C++ library that your project depends on.  Modifications can include:
- Changing compilation flags
- Changing installation strategies
- Changing source code
- Changing static data

This tutorial demonstrates how to apply these changes to a dependency installed with vcpkg in a way that's **cheap,** **shareable,** and **permanent**.

The full process can be completed in 15 minutes.
## Project Structure

We'll use a mock project structure, like so:

```
project/
 |-- include/
 |-- src/
 |-- vcpkg/
 |-- CMakeLists.txt
 |-- README.md
```

In this example, we'd likely set up the vcpkg CLI utility, like so:

```sh
# pwd: at project root
chmod +x ./vcpkg/bootstrap-vcpkg.sh
./vcpkg/bootstrap-vcpkg.sh
```

## Step (1 of 4): Create a Local Registry

The first change we'll make to the project structure is making a directory to hold our vcpkg portfiles.

> A [Port](https://learn.microsoft.com/en-us/vcpkg/concepts/ports#portfile-portfilecmake) is a small, cheap collection of metadata files which instruct vcpkg on how to download, configure, build, and install your dependencies.

To customize our dependency, we're going to copy its portfiles and place them in our own custom registry.

First, let's create a directory to hold our vcpkg portfiles:

```shell
# pwd: at project root
mkdir registry
```

Call it whatever you'd like.  This tutorial refers to it as a **local registry.**

## Step (2 of 4): Copy the Port Files

This tutorial will use the fake "foobaz" package as an example.

```sh
# pwd: at project root
./vcpkg/vcpkg install foobaz
```

We're going to copy the `foobaz` portfiles into our **local registry** in order to apply our modifications in the future.

```sh
# pwd: at project root
cp -r ./vcpkg/ports/foobaz ./registry/foobaz
```

Take a look inside `./registry/foobaz` and you'll probably see three files:

```
project/
 |-- registry/
 |    |-- foobaz/
 |    |    |-- build_fixes.patch
 |    |    |-- portfile.cmake
 |    |    |-- vcpkg.json
```

Let's examine each of them.
- `build_fixes.patch` is a **git diff** that vcpkg applies to your dependency's source tree before compiling and installing it.
- `portfile.cmake` is a **cmake script** that tells vcpkg where to find your dependency's source tree, plus some other stuff.
- `vcpkg.json` is a **metadata file** that's conceptually similar to a package.json file from the Node.js universe.

It's recommended to keep `vcpkg.json` the same.

This tutorial is going to demonstrate how to generate a new `build_fixes.patch` file, and it will contain the modifications you want to apply to the source code, build configuration, etc.

> **Please note** that `portfile.cmake` is run in CMake's script mode, so we **cannot** use this to change build parameters for our dependency.  Those must be modified from within the `build_fixes.patch` instead.

## Step (3 of 4): Generating a Patch File

Awesome, it's time to make some changes!

First, clone the dependency's source tree, so we can make our changes.  This is a temporary folder that won't be checked into your git project.  I'm cloning inside of `registry/foobaz` for convenience, but you may put it anywhere you'd like.

```sh
# pwd: at project/registry/foobaz
git clone https://github.com/username/foobaz.git source
```

Next, we're going to apply the existing `build_fixes.patch` file on top of the source tree.  This puts our dependency's source into the state that vcpkg uses by default when you install it.

```sh
# pwd: at project/registry/foobaz/source
git apply ../build_fixes.patch
```

Now, it's time to make your changes!  You can modify the `CMakeLists.txt` to adjust build configuration, comment out source files, introduce new code, whatever the hell you want.

![Dancing Cat GIF](https://media0.giphy.com/media/v1.Y2lkPTc5MGI3NjExazZsd2RmbHBrczBoYzlxcWdpYnkxdjgxcHhoZnJiZDhieHdhMWNqdSZlcD12MV9pbnRlcm5hbF9naWZfYnlfaWQmY3Q9Zw/l0HlDUUzqPf4V17HO/giphy.webp)

After applying your modifications, create a new **git diff** file.  This overwrites the existing `build_fixes.patch` with the changes you've just made (plus whatever was already there).

```sh
# pwd: at project/registry/foobaz/source
git diff > ../build_fixes.patch
```

Remove the source tree when you're done.

```sh
# pwd: at project/registry/foobaz
rm -rf source
```

## Finale (4 of 4): Use your Modified Dependency

It is necessary to remove `foobaz` from vcpkg so that we can re-install our modified version.

```sh
# pwd: at project/
./vcpkg/vcpkg remove foobaz
```

**From now on**, to make vcpkg download your modified dependency, the CLI tool must be invoked like so:

```sh
# pwd: at project/
./vcpkg/vcpkg install foobaz --overlay-ports=registry/foobaz
```

Check it out!  **You're done!**
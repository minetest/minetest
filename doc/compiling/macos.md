# Compiling on MacOS

## Requirements

- [Git](https://git-scm.com/downloads)
- [Pixi](https://pixi.sh/)

## Download

Download source (this is the URL to the latest of source repository, which might not work at all times) using Git:

```bash
git clone --depth 1 https://github.com/minetest/minetest.git
cd minetest
git submodule update --init --recursive
```

## Build

```bash
pixi shell
cmake -B build -S . \
    -DCMAKE_FIND_FRAMEWORK=FIRST \
    -DCMAKE_INSTALL_PREFIX=$(pwd)/build/macos/ \
    -GNinja \
    -DRUN_IN_PLACE=FALSE  \
    -DENABLE_GETTEXT=TRUE \
    -DINSTALL_DEVTEST=TRUE \
    -DINSTALL_MINETEST_GAME=TRUE \
    -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations" \
    -DFREETYPE_LIBRARY="${CONDA_PREFIX}/lib/libfreetype.dylib" \
    -DICONV_LIBRARY="${CONDA_PREFIX}/lib/libiconv.dylib" \
    -DCMAKE_INSTALL_RPATH="${CONDA_PREFIX}/lib" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1
cmake --build build
cmake --install build

# arm64 Macs w/ MacOS >= BigSur
codesign --force --deep -s - build/macos/minetest.app
```

## Run

```bash
./build/macos/minetest.app/Contents/MacOS/minetest
```

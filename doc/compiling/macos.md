# Compiling on MacOS

Note: the conda environment.yml file currently doesn't work on arm64,
and that's what's tested on the CI (which uses x86-64).
So these instructions should work aren't tested.

## Requirements

- [Homebrew](https://brew.sh/)
- [Git](https://git-scm.com/downloads)

Install dependencies with homebrew:

```
brew install cmake capnp freetype gettext gmp hiredis jpeg jsoncpp leveldb libogg libpng libvorbis luajit zstd gettext sdl2 zeromq zmqpp ninja
```

## Download

Download source (this is the URL to the latest of source repository, which might not work at all times) using Git:

```bash
git clone --depth 1 https://github.com/minetest/minetest.git
cd minetest
git submodule update --init --recursive
```

## Build

Note the `-DICONV_LIBRARY` should only be set if a conda environment is active
(even an empty conda environment seems to have libiconv which conflicts with system iconv).

```bash
cmake -B build -S . \
    -DCMAKE_FIND_FRAMEWORK=LAST \
    -DCMAKE_INSTALL_PREFIX=$(pwd)/build/macos/ \
    -GNinja \
    -DRUN_IN_PLACE=FALSE  \
    -DENABLE_GETTEXT=TRUE \
    -DINSTALL_DEVTEST=TRUE \
    -DINSTALL_MINETEST_GAME=TRUE \
    -DCMAKE_CXX_FLAGS="-Wno-deprecated-declarations" \
    -DICONV_LIBRARY="${CONDA_PREFIX}/lib/libiconv.dylib"
cmake --build build
cmake --install build

# arm64 Macs w/ MacOS >= BigSur
codesign --force --deep -s - build/macos/minetest.app
```

## Run

```bash
./build/macos/minetest.app/Contents/MacOS/minetest
```

# Compiling on MacOS

## Requirements

- [Homebrew](https://brew.sh/)
- [Git](https://git-scm.com/downloads)

Install dependencies with homebrew:

```
brew install cmake freetype gettext gmp hiredis jpeg jsoncpp leveldb libogg libpng libvorbis luajit zstd gettext
```

## Download

Download source (this is the URL to the latest of source repository, which might not work at all times) using Git:

```bash
git clone --depth 1 https://github.com/minetest/minetest.git
cd minetest
```

Download Minetest's fork of Irrlicht:

```
git clone --depth 1 https://github.com/minetest/irrlicht.git lib/irrlichtmt
```

## Build

```bash
mkdir build
cd build

cmake .. \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
    -DCMAKE_FIND_FRAMEWORK=LAST \
    -DCMAKE_INSTALL_PREFIX=../build/macos/ \
    -DRUN_IN_PLACE=FALSE -DENABLE_GETTEXT=TRUE

make -j$(sysctl -n hw.logicalcpu)
make install
```

## Run

```
open ./build/macos/minetest.app
```

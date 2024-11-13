# Setup

Install [pixi](https://pixi.sh/latest/).

```bash
# Once per computer:
# To match the toolchain that rattler-build uses,
# set up GCC-13:
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
# minetest deps. Only the ones not available in conda.
sudo apt install gcc-13 g++-13 libgl1-mesa-dev -yq
# make GCC-13 the default
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 60 --slave /usr/bin/g++ g++ /usr/bin/g++-13

git clone git@github.com:Astera-org/minetest.git
cd minetest
git submodule update --init --recursive

# activate pixi environment
pixi shell -e gym-test

cmake -B build -S . \
	-DCMAKE_FIND_FRAMEWORK=LAST \
	-DRUN_IN_PLACE=TRUE \
	-DENABLE_SOUND=FALSE \
	-DENABLE_GETTEXT=TRUE \
	-GNinja \
	-DUSE_SDL2=ON \
	-DCMAKE_CXX_FLAGS="-fuse-ld=mold" \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 && \
	cmake --build build

pushd minetest-gymnasium && pytest .; popd
```

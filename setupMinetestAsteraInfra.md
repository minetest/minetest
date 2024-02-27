# Setup

This should work on a standard infra machine. Last tested on `quick-weevil`.

Requires conda to be installed.

```bash
set -euox pipefail

# To match the toolchain that rattler-build uses,
# set up GCC-13:
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
# minetest deps. Only the ones not available in conda.
sudo apt install gcc-13 g++-13 libgl1-mesa-dev mold xorg-dev -yq
# make GCC-13 the default
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 60 --slave /usr/bin/g++ g++ /usr/bin/g++-13

git clone git@github.com:Astera-org/minetest.git
cd minetest

# activate conda environment
mamba env create && conda activate minetest

git submodule update --init --recursive

cmake -B build -S . \
	-DCMAKE_FIND_FRAMEWORK=LAST \
	-DRUN_IN_PLACE=TRUE \
	-DENABLE_SOUND=FALSE \
	-DENABLE_GETTEXT=TRUE \
	-GNinja \
	-DCMAKE_CXX_FLAGS="-fuse-ld=mold" \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 && \
	cmake --build build

pushd python && pytest .; popd
```

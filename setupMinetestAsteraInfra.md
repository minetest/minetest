# Setup

This should work on a standard infra machine. Last tested on `quick-weevil`.

Requires conda to be installed.

```bash
set -euox pipefail

# BEGIN unnecessary if using dev container

# minetest deps. Only the ones not available in conda.
sudo apt install g++ libgl1-mesa-dev mold -yq

git clone git@github.com:Astera-org/minetest.git
cd minetest

# activate conda environment
mamba env create && conda activate minetest

# END unnecessary if using dev container
git submodule update --init --recursive

cmake -B build -S . \
	-DCMAKE_FIND_FRAMEWORK=LAST \
	-DRUN_IN_PLACE=TRUE \
	-DENABLE_SOUND=FALSE \
	-DENABLE_GETTEXT=TRUE \
	-DBUILD_HEADLESS=TRUE \
	-GNinja \
	-DCMAKE_CXX_FLAGS="-fuse-ld=mold" \
	-DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=1 && \
	cmake --build build

pushd python && pytest .; popd
```

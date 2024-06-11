#!/bin/bash -eu
cd $HOME
git clone --depth 1 https://github.com/LuaJIT/LuaJIT
pushd LuaJIT
make -j$(nproc)
popd

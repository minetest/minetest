#! /bin/bash -eu

# Use ccache if it is available
extra_args=()
command -v ccache >/dev/null && extra_args+=(-DCMAKE_{C,CXX}_COMPILER_LAUNCHER=ccache)

cd /tmp
git clone --recursive https://github.com/jupp0r/prometheus-cpp
mkdir prometheus-cpp/build
cd prometheus-cpp/build
cmake .. \
	"${extra_args[@]}" \
	-DCMAKE_INSTALL_PREFIX=/usr/local \
	-DCMAKE_BUILD_TYPE=Release \
	-DENABLE_TESTING=0
make -j$(nproc)
sudo make install


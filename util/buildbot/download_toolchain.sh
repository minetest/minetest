#!/bin/bash
set -e
topdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [[ -z "$1" || -z "$2" ]]; then
	echo "Usage: $0 <i686 | x86_64> <dest path>"
	exit 1
fi

# our current toolchain:
# binutils 2.41 + GCC 13.2.0 + Mingw-w64 11.0.1 with UCRT enabled and winpthreads support
# built from source on Ubuntu 22.04, so should work on any similarily up-to-date distro
ver=13.2.0
os=ubuntu22.04
name="mingw-w64-${1}_${ver}_${os}.tar.xz"
wget "http://minetest.kitsunemimi.pw/$name" -O "$name"
sha256sum -w -c <(grep -F "$name" "$topdir/sha256sums.txt")
tar -xaf "$name" -C "$2"
rm -f "$name"

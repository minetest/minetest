#!/bin/bash
set -e
topdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [[ -z "$1" || -z "$2" ]]; then
	echo "Usage: $0 <i686 | x86_64> <dest path>"
	exit 1
fi

ver=11.2.0
os=ubuntu20.04
name="mingw-w64-${1}_${ver}_${os}.tar.xz"
wget "http://minetest.kitsunemimi.pw/$name" -O "$name"
sha256sum -w -c <(grep -F "$name" "$topdir/sha256sums.txt")
tar -xaf "$name" -C "$2"
rm -f "$name"

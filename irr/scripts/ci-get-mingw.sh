#!/bin/bash
set -e
topdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

name=llvm-mingw-20231128-ucrt-ubuntu-20.04-x86_64.tar.xz
wget "https://github.com/mstorsjo/llvm-mingw/releases/download/20231128/$name" -O "$name"
sha256sum -w -c <(grep -F "$name" "$topdir/sha256sums.txt")
sudo tar -xaf "$name" -C /usr --strip-components=1
rm -f "$name"

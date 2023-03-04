#!/bin/bash -e

# Linux build only
install_linux_deps() {
	local pkgs=(
		cmake gettext postgresql
		libpng-dev libjpeg-dev libxi-dev libgl1-mesa-dev
		libsqlite3-dev libhiredis-dev libogg-dev libgmp-dev libvorbis-dev
		libopenal-dev libpq-dev libleveldb-dev libcurl4-openssl-dev libzstd-dev
	)

	if [[ "$1" == "--no-irr" ]]; then
		shift
	else
		local ver=$(cat misc/irrlichtmt_tag.txt)
		wget "https://github.com/minetest/irrlicht/releases/download/$ver/ubuntu-bionic.tar.gz"
		sudo tar -xaf ubuntu-bionic.tar.gz -C /usr/local
	fi

	sudo apt-get update
	sudo apt-get install -y --no-install-recommends "${pkgs[@]}" "$@"

	sudo systemctl start postgresql.service
	sudo -u postgres psql <<<"
		CREATE USER minetest WITH PASSWORD 'minetest';
		CREATE DATABASE minetest;
	"
}

# macOS build only
install_macos_deps() {
	local pkgs=(
		cmake gettext freetype gmp jpeg-turbo jsoncpp leveldb
		libogg libpng libvorbis luajit zstd
	)
	brew update
	brew install "${pkgs[@]}"
	brew unlink $(brew ls --formula)
	brew link "${pkgs[@]}"
}

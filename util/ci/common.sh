#!/bin/bash -e

set -x

# Linux build only
install_linux_deps() {
	local pkgs=(
		postgresql
		libgl1-mesa-dev
		libhiredis-dev libogg-dev libgmp-dev libvorbis-dev
		libopenal-dev libpq-dev libleveldb-dev
	)

	if [[ "$1" == "--no-irr" ]]; then
		shift
	else
		local ver=$(cat misc/irrlichtmt_tag.txt)
		wget "https://github.com/minetest/irrlicht/releases/download/$ver/ubuntu-bionic.tar.gz"
		sudo tar -xaf ubuntu-bionic.tar.gz -C /usr/local
	fi

	sudo apt-get update
	sudo apt-get install -y "${pkgs[@]}" "$@"

	sudo systemctl start postgresql.service
	sudo -u postgres psql <<<"
		CREATE USER minetest WITH PASSWORD 'minetest';
		CREATE DATABASE minetest;
	"

	git submodule update --init --recursive
}

# macOS build only
install_macos_deps() {
	local pkgs=(
		leveldb libogg libvorbis
	)
	export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
	export HOMEBREW_NO_INSTALL_CLEANUP=1
	# contrary to how it may look --auto-update makes brew do *less*
	brew update --auto-update
	brew install --display-times "${pkgs[@]}"
	brew unlink $(brew ls --formula)
	brew link "${pkgs[@]}"

	git submodule update --init --recursive
}

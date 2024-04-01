/*
Minetest
Copyright (C) 2024 rubenwardy

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <string>
#include "irrlichttypes.h"
#include "ITexture.h"
#include "config.h"
#include "irr_ptr.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"

#if USE_CURL

/**
 * A component to load a single texture from the web. After calling load,
 * update() must be called until the state is not Loading.
 */
class RemoteTexture
{
public:
	RemoteTexture(video::IVideoDriver *driver, io::IFileSystem *fsys):
		driver(driver), fsys(fsys)
	{}
	RemoteTexture(const RemoteTexture &other) = delete;
	RemoteTexture(RemoteTexture &&other) = default;

	void load(const std::string &url);
	void update();

	enum State {
		Empty,
		Loading,
		Success,
		Failure
	};

	inline irr_ptr<video::ITexture> get() const { return texture; }
	inline State getState() const { return state; }

private:
	video::IVideoDriver *driver;
	io::IFileSystem *fsys;
	State state = Empty;
	std::string cache_path;
	std::string requested_url;
	u64 caller_handle;
	irr_ptr<video::ITexture> texture;
};

#endif

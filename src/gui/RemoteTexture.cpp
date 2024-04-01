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

#include "RemoteTexture.h"

#if USE_CURL

#include "httpfetch.h"
#include "settings.h"
#include "log.h"
#include <fstream>
#include "porting.h"
#include "client/renderingengine.h"
#include "filesys.h"
#include "util/base64.h"
#include <IWriteFile.h>

namespace {
	std::string GetExtensionFromURL(const std::string &url)
	{
		auto idx = url.rfind('.');
		if (idx == std::string::npos) {
			return "";
		}

		auto ret = url.substr(idx);
		if (ret.find('/') != std::string::npos) {
			return "";
		}

		return ret;
	}
}

void RemoteTexture::load(const std::string &url)
{
	if (url == requested_url)
		return;

	cache_path = porting::path_cache + DIR_DELIM + base64_encode(url) +
			GetExtensionFromURL(url);
	if (fs::IsFile(cache_path)) {
		errorstream << "Loading " << url << " from cache: " << cache_path << std::endl;
		auto raw_texture = driver->getTexture(cache_path.c_str());
		if (raw_texture) {
			texture = ::grab(raw_texture);
			state = Success;
		} else {
			state = Failure;
		}
		return;
	}

	errorstream << "Fetching " << url << ", cache path: " << cache_path << std::endl;

	HTTPFetchRequest req;
	req.caller = httpfetch_caller_alloc_secure();
	req.url = url;
	req.extra_headers.emplace_back("Accept: image/png, image/jpeg");
	req.timeout = std::max(MIN_HTTPFETCH_TIMEOUT,
			(long)g_settings->getS32("curl_file_download_timeout"));

	httpfetch_async(req);
	state = Loading;
	requested_url = url;
	caller_handle = req.caller;
	texture = {};
}

void RemoteTexture::update()
{
	if (texture || state != State::Loading)
		return;

	HTTPFetchResult res;
	bool completed = httpfetch_async_get(caller_handle, res);
	if (!completed)
		return;

	if (!res.succeeded) {
		state = Failure;
		return;
	}

	auto rfile = irr_ptr<io::IReadFile>(fsys->createMemoryReadFile(
			res.data.c_str(), res.data.size(), "_tempreadfile"));
	FATAL_ERROR_IF(!rfile, "Could not create irrlicht memory file.");

	{
		auto wfile = irr_ptr<io::IWriteFile>(fsys->createAndWriteFile(cache_path.c_str()));
		wfile->write(res.data.c_str(), res.data.size());

		errorstream << "Wrote cache file at: " << cache_path << std::endl;
	}

	// Read image
	video::IImage *img = driver->createImageFromFile(rfile.get());
	if (!img) {
		errorstream<<"Client: Cannot create image from url \""<< requested_url <<"\""<<std::endl;
		state = Failure;
		return;
	}

	video::ITexture *raw_texture = driver->addTexture(requested_url.c_str(), img);
	if (!raw_texture) {
		errorstream<<"Client: Unable to load texture into memory, url = \""<< requested_url <<"\""<<std::endl;
		img->drop();
		state = Failure;
		return;
	}

	texture = ::grab(raw_texture);
	state = Success;
	return;
}

#endif

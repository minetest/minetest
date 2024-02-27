/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "texturepaths.h"

#include "util/container.h"
#include "settings.h"
#include "filesys.h"
#include "porting.h"

/*
	A cache from texture name to texture path
*/
static MutexedMap<std::string, std::string> g_texturename_to_path_cache;

/*
	Replaces the filename extension.
	eg:
		std::string image = "a/image.png"
		replace_ext(image, "jpg")
		-> image = "a/image.jpg"
	Returns true on success.
*/
static bool replace_ext(std::string &path, const char *ext)
{
	if (ext == NULL)
		return false;
	// Find place of last dot, fail if \ or / found.
	s32 last_dot_i = -1;
	for (s32 i=path.size()-1; i>=0; i--)
	{
		if (path[i] == '.')
		{
			last_dot_i = i;
			break;
		}

		if (path[i] == '\\' || path[i] == '/')
			break;
	}
	// If not found, return an empty string
	if (last_dot_i == -1)
		return false;
	// Else make the new path
	path = path.substr(0, last_dot_i+1) + ext;
	return true;
}

/*
	Find out the full path of an image by trying different filename
	extensions.
	If failed, return "".
*/
std::string getImagePath(std::string path)
{
	// A NULL-ended list of possible image extensions
	const char *extensions[] = { "png", "jpg", "bmp", "tga", NULL };
	// If there is no extension, assume PNG
	if (removeStringEnd(path, extensions).empty())
		path = path + ".png";
	// Check paths until something is found to exist
	const char **ext = extensions;
	do{
		bool r = replace_ext(path, *ext);
		if (!r)
			return "";
		if (fs::PathExists(path))
			return path;
	}
	while((++ext) != NULL);

	return "";
}

/*
	Gets the path to a texture by first checking if the texture exists
	in texture_path and if not, using the data path.
	Checks all supported extensions by replacing the original extension.
	If not found, returns "".
	Utilizes a thread-safe cache.
*/
std::string getTexturePath(const std::string &filename, bool *is_base_pack)
{
	std::string fullpath;

	// This can set a wrong value on cached textures, but is irrelevant because
	// is_base_pack is only passed when initializing the textures the first time
	if (is_base_pack)
		*is_base_pack = false;
	/*
		Check from cache
	*/
	bool incache = g_texturename_to_path_cache.get(filename, &fullpath);
	if (incache)
		return fullpath;

	/*
		Check from texture_path
	*/
	for (const auto &path : getTextureDirs()) {
		std::string testpath = path + DIR_DELIM;
		testpath.append(filename);
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
		if (!fullpath.empty())
			break;
	}

	/*
		Check from default data directory
	*/
	if (fullpath.empty())
	{
		std::string base_path = porting::path_share + DIR_DELIM + "textures"
				+ DIR_DELIM + "base" + DIR_DELIM + "pack";
		std::string testpath = base_path + DIR_DELIM + filename;
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
		if (is_base_pack && !fullpath.empty())
			*is_base_pack = true;
	}

	// Add to cache (also an empty result is cached)
	g_texturename_to_path_cache.set(filename, fullpath);

	// Finally return it
	return fullpath;
}

void clearTextureNameCache()
{
	g_texturename_to_path_cache.clear();
}

std::vector<std::string> getTextureDirs()
{
	return fs::GetRecursiveDirs(g_settings->get("texture_path"));
}

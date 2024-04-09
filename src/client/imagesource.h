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

#pragma once

#include <IImage.h>
#include <string>
#include "settings.h"

// This file is only used for internal generation of images.
// Use texturesource.h to handle textures.

// A cache used for storing source images.
// (A "source image" is an unmodified image directly taken from the filesystem.)
// Does not contain modified images.
class SourceImageCache {
public:
	~SourceImageCache();

	void insert(const std::string &name, video::IImage *img, bool prefer_local);

	video::IImage* get(const std::string &name);

	// Primarily fetches from cache, secondarily tries to read from filesystem.
	video::IImage *getOrLoad(const std::string &name);
private:
	std::map<std::string, video::IImage*> m_images;
};

// Generates images using texture modifiers, and caches source images.
struct ImageSource {
	/*! Generates an image from a full string like
	 * "stone.png^mineral_coal.png^[crack:1:0".
	 * The returned Image should be dropped.
	 * source_image_names is important to determine when to flush the image from a cache (dynamic media)
	 */
	video::IImage* generateImage(std::string_view name, std::set<std::string> &source_image_names);

	// Insert a source image into the cache without touching the filesystem.
	void insertSourceImage(const std::string &name, video::IImage *img, bool prefer_local);

	// TODO should probably be moved elsewhere
	static video::SColor getImageAverageColor(const video::IImage &image);

	ImageSource() :
		m_setting_mipmap{g_settings->getBool("mip_map")},
		m_setting_trilinear_filter{g_settings->getBool("trilinear_filter")},
		m_setting_bilinear_filter{g_settings->getBool("bilinear_filter")},
		m_setting_anisotropic_filter{g_settings->getBool("anisotropic_filter")}
	{};

private:

	// Generate image based on a string like "stone.png" or "[crack:1:0".
	// If baseimg is NULL, it is created. Otherwise stuff is made on it.
	// source_image_names is important to determine when to flush the image from a cache (dynamic media).
	bool generateImagePart(std::string_view part_of_name, video::IImage *& baseimg,
			std::set<std::string> &source_image_names);

	// Cached settings needed for making textures from meshes
	bool m_setting_mipmap;
	bool m_setting_trilinear_filter;
	bool m_setting_bilinear_filter;
	bool m_setting_anisotropic_filter;

	// Cache of source images
	SourceImageCache m_sourcecache;
};

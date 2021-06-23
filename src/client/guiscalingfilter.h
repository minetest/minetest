/*
Copyright (C) 2015 Aaron Suen <warr1024@gmail.com>

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

#include "irrlichttypes_extrabloated.h"

/* Manually insert an image into the cache, useful to avoid texture-to-image
 * conversion whenever we can intercept it.
 */
void guiScalingCache(const io::path &key, video::IVideoDriver *driver, video::IImage *value);

// Manually clear the cache, e.g. when switching to different worlds.
void guiScalingCacheClear();

/* Get a cached, high-quality pre-scaled texture for display purposes.  If the
 * texture is not already cached, attempt to create it.  Returns a pre-scaled texture,
 * or the original texture if unable to pre-scale it.
 */
video::ITexture *guiScalingResizeCached(video::IVideoDriver *driver, video::ITexture *src,
		const core::rect<s32> &srcrect, const core::rect<s32> &destrect);

/* Convenience wrapper for guiScalingResizeCached that accepts parameters that
 * are available at GUI imagebutton creation time.
 */
video::ITexture *guiScalingImageButton(video::IVideoDriver *driver, video::ITexture *src,
		s32 width, s32 height);

/* Replacement for driver->draw2DImage() that uses the high-quality pre-scaled
 * texture, if configured.
 */
void draw2DImageFilterScaled(video::IVideoDriver *driver, video::ITexture *txr,
		const core::rect<s32> &destrect, const core::rect<s32> &srcrect,
		const core::rect<s32> *cliprect = 0, const video::SColor *const colors = 0,
		bool usealpha = false);

/*
 * 9-slice / segment drawing
 */
void draw2DImage9Slice(video::IVideoDriver *driver, video::ITexture *texture,
		const core::rect<s32> &rect, const core::rect<s32> &middle,
		const core::rect<s32> *cliprect = nullptr,
		const video::SColor *const colors = nullptr);

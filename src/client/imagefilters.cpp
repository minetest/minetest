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

#include "imagefilters.h"
#include "irr_v2d.h"
#include "log.h"
#include "util/numeric.h"
#include <array>
#include <cmath>
#include <cassert>
#include <deque>
#include <limits>
#include <utility>
#include <vector>
#include <algorithm>

// Simple 2D bitmap class with just the functionality needed here
class Bitmap {
	u32 linesize, lines;
	std::vector<bool> data;

public:
	Bitmap(u32 width, u32 height) :  linesize(width), lines(height),
		data(width * height) {}

	inline bool get(u32 x, u32 y) const {
		return data[y * linesize + x];
	}

	inline void set(u32 x, u32 y) {
		data[y * linesize + x] = true;
	}

	inline void copy(Bitmap &to) const {
		assert(to.linesize == linesize && to.lines == lines);
		to.data = data;
	}
};

// FIXME benchmark this
template <bool IS_A8R8G8B8>
static void imageCleanTransparentWithInlining(video::IImage *src, u32 threshold)
{
	void *const src_data = src->getData();
	const core::dimension2d<u32> dim = src->getDimension();

	assert(dim.Height <= std::numeric_limits<u16>::max() &&
			dim.Width <= std::numeric_limits<u16>::max());
	const u16 w = dim.Width, h = dim.Height;

	auto get_pixel = [=](u16 x, u16 y) -> video::SColor {
		if constexpr (IS_A8R8G8B8) {
			return reinterpret_cast<u32 *>(src_data)[y*w + x];
		} else {
			return src->getPixel(x, y);
		}
	};
	auto set_pixel = [=](u16 x, u16 y, video::SColor color) {
		if constexpr (IS_A8R8G8B8) {
			u32 *dest = &reinterpret_cast<u32 *>(src_data)[y*w + x];
			*dest = color.color;
		} else {
			src->setPixel(x, y, color);
		}
	};

	Bitmap seen(w, h);
	Bitmap prev_levels(w, h);
	std::vector<std::pair<u16, u16>> level, next_level;

	// First pass: Discover all transparent pixels adjacent to opaque pixels.
	// Note: loop y around x for better cache locality.
	for (u16 ctry = 0; ctry < h; ++ctry)
	for (u16 ctrx = 0; ctrx < w; ++ctrx) {
		if (get_pixel(ctrx, ctry).getAlpha() < threshold)
			continue;
		seen.set(ctrx, ctry);
		prev_levels.set(ctrx, ctry);
		v2s32 pos(ctrx, ctry);
		// Walk each neighbor pixel (clipped to image bounds)
		for (u16 sy = (ctry < 1) ? 0 : (ctry - 1);
				sy <= (ctry + 1) && sy < h; sy++)
		for (u16 sx = (ctrx < 1) ? 0 : (ctrx - 1);
				sx <= (ctrx + 1) && sx < w; sx++) {
			if (seen.get(sx, sy))
				continue;
			if (get_pixel(sx, sy).getAlpha() >= threshold)
				continue;
			seen.set(sx, sy);
			level.emplace_back(sx, sy);
		}
	}

	while (!level.empty()) {
		next_level.clear();
		for (auto pos : level) {
			const auto x = pos.first, y = pos.second;
			struct { u32 r, g, b, a; } sum {0, 0, 0, 0};
			// Walk each neighbor pixel (clipped to image bounds)
			for (u16 sy = (y < 1) ? 0 : (y - 1);
					sy <= (y + 1) && sy < dim.Height; sy++)
			for (u16 sx = (x < 1) ? 0 : (x - 1);
					sx <= (x + 1) && sx < dim.Width; sx++) {
				auto color = get_pixel(sx, sy);
				if (!seen.get(sx, sy)) {
					seen.set(sx, sy);
					next_level.emplace_back(sx, sy);
					continue;
				}
				if (!prev_levels.get(sx, sy))
					continue;
				// FIXME we should not be weighing by alpha when we consider clipping.
				// This way, propagated colors - even if originating from low alpha colors -
				// would have higher influence than low alpha colors above the threshold.
				const u32 a = color.getAlpha() >= threshold ? color.getAlpha() : 255;
				// FIXME not gamma-correct
				sum.r += color.getRed() * a;
				sum.g += color.getGreen() * a;
				sum.b += color.getBlue() * a;
				sum.a += a;
			}
			assert(sum.a >= threshold);
			// Set pixel to average weighted by alpha
			video::SColor c = get_pixel(x, y);
			c.setRed(sum.r / sum.a);
			c.setGreen(sum.g / sum.a);
			c.setBlue(sum.b / sum.a);
			set_pixel(x, y, c);
		}
		for (auto pos : level)
			prev_levels.set(pos.first, pos.second);
		level.swap(next_level);
	}
}

/* Fill in RGB values for transparent pixels, to correct for odd colors
 * appearing at borders when blending.  This is because many PNG optimizers
 * like to discard RGB values of transparent pixels, but when blending then
 * with non-transparent neighbors, their RGB values will show up nonetheless.
 *
 * This function modifies the original image in-place.
 *
 * Parameter "threshold" is the alpha level below which pixels are considered
 * transparent. Should be 127 when the texture is used with ALPHA_CHANNEL_REF,
 * 0 when alpha blending is used.
 */
void imageCleanTransparent(video::IImage *src, u32 threshold)
{
	if (src->getColorFormat() == video::ECF_A8R8G8B8)
		imageCleanTransparentWithInlining<true>(src, threshold);
	else
		imageCleanTransparentWithInlining<false>(src, threshold);
}

/* Scale a region of an image into another image, using nearest-neighbor with
 * anti-aliasing; treat pixels as crisp rectangles, but blend them at boundaries
 * to prevent non-integer scaling ratio artifacts.  Note that this may cause
 * some blending at the edges where pixels don't line up perfectly, but this
 * filter is designed to produce the most accurate results for both upscaling
 * and downscaling.
 */
void imageScaleNNAA(video::IImage *src, const core::rect<s32> &srcrect, video::IImage *dest)
{
	double sx, sy, minsx, maxsx, minsy, maxsy, area, ra, ga, ba, aa, pw, ph, pa;
	u32 dy, dx;
	video::SColor pxl;

	// Cache rectangle boundaries.
	double sox = srcrect.UpperLeftCorner.X * 1.0;
	double soy = srcrect.UpperLeftCorner.Y * 1.0;
	double sw = srcrect.getWidth() * 1.0;
	double sh = srcrect.getHeight() * 1.0;

	// Walk each destination image pixel.
	// Note: loop y around x for better cache locality.
	core::dimension2d<u32> dim = dest->getDimension();
	for (dy = 0; dy < dim.Height; dy++)
	for (dx = 0; dx < dim.Width; dx++) {

		// Calculate floating-point source rectangle bounds.
		// Do some basic clipping, and for mirrored/flipped rects,
		// make sure min/max are in the right order.
		minsx = sox + (dx * sw / dim.Width);
		minsx = rangelim(minsx, 0, sox + sw);
		maxsx = minsx + sw / dim.Width;
		maxsx = rangelim(maxsx, 0, sox + sw);
		if (minsx > maxsx)
			SWAP(double, minsx, maxsx);
		minsy = soy + (dy * sh / dim.Height);
		minsy = rangelim(minsy, 0, soy + sh);
		maxsy = minsy + sh / dim.Height;
		maxsy = rangelim(maxsy, 0, soy + sh);
		if (minsy > maxsy)
			SWAP(double, minsy, maxsy);

		// Total area, and integral of r, g, b values over that area,
		// initialized to zero, to be summed up in next loops.
		area = 0;
		ra = 0;
		ga = 0;
		ba = 0;
		aa = 0;

		// Loop over the integral pixel positions described by those bounds.
		for (sy = floor(minsy); sy < maxsy; sy++)
		for (sx = floor(minsx); sx < maxsx; sx++) {

			// Calculate width, height, then area of dest pixel
			// that's covered by this source pixel.
			pw = 1;
			if (minsx > sx)
				pw += sx - minsx;
			if (maxsx < (sx + 1))
				pw += maxsx - sx - 1;
			ph = 1;
			if (minsy > sy)
				ph += sy - minsy;
			if (maxsy < (sy + 1))
				ph += maxsy - sy - 1;
			pa = pw * ph;

			// Get source pixel and add it to totals, weighted
			// by covered area and alpha.
			pxl = src->getPixel((u32)sx, (u32)sy);
			area += pa;
			ra += pa * pxl.getRed();
			ga += pa * pxl.getGreen();
			ba += pa * pxl.getBlue();
			aa += pa * pxl.getAlpha();
		}

		// Set the destination image pixel to the average color.
		if (area > 0) {
			pxl.setRed(ra / area + 0.5);
			pxl.setGreen(ga / area + 0.5);
			pxl.setBlue(ba / area + 0.5);
			pxl.setAlpha(aa / area + 0.5);
		} else {
			pxl.setRed(0);
			pxl.setGreen(0);
			pxl.setBlue(0);
			pxl.setAlpha(0);
		}
		dest->setPixel(dx, dy, pxl);
	}
}

/* Check and align image to npot2 if required by hardware
 * @param image image to check for npot2 alignment
 * @param driver driver to use for image operations
 * @return image or copy of image aligned to npot2
 */
video::IImage *Align2Npot2(video::IImage *image, video::IVideoDriver *driver)
{
	if (image == nullptr)
		return image;

	if (driver->queryFeature(video::EVDF_TEXTURE_NPOT))
		return image;

	core::dimension2d<u32> dim = image->getDimension();
	unsigned int height = npot2(dim.Height);
	unsigned int width  = npot2(dim.Width);

	if (dim.Height == height && dim.Width == width)
		return image;

	if (dim.Height > height)
		height *= 2;
	if (dim.Width > width)
		width *= 2;

	video::IImage *targetimage =
			driver->createImage(video::ECF_A8R8G8B8,
					core::dimension2d<u32>(width, height));

	if (targetimage != nullptr)
		image->copyToScaling(targetimage);
	image->drop();
	return targetimage;
}

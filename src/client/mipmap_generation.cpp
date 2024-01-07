/*
Minetest
Copyright (C) 2023 HybridDog

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

#include "mipmap_generation.h"

#include <array>
#include <vector>
#include <memory>
#include <stdexcept>
#include <IImage.h>

// Square root of the patch size; recommended: 2
constexpr u8 SQR_NP{2};
// Mixing ratio between simple averaging and perceptual downscaling
constexpr f32 LINEAR_RATIO{0.5f};


#define CLAMP(V, A, B) (V) < (A) ? (A) : (V) > (B) ? (B) : (V)
#define MIN(V, R) ((V) < (R) ? (V) : (R))
#define MAX(V, R) ((V) > (R) ? (V) : (R))
#define INDEX(X, Y, STRIDE) ((Y) * (STRIDE) + (X))


namespace {

/*! \brief A helper struct to handle 2D floating-point data
 */
struct Matrix {
	u32 w;
	u32 h;
	std::unique_ptr<f32[]> data;
	Matrix(u32 width, u32 height):
		w{width},
		h{height},
		data{std::unique_ptr<f32[]>{new f32[width * height]}}
	{}
};

/*! \brief sRGB OETF
 *
 * Taken from https://github.com/tobspr/GLSL-Color-Spaces/
 */
f32 linear_to_srgb(f32 v)
{
	if (v > 0.0031308f)
		return 1.055f * powf(v, 1.0f / 2.4f) - 0.055f;
	return 12.92f * v;
}

/*! \brief sRGB EOTF
 *
 * Taken from https://github.com/tobspr/GLSL-Color-Spaces/
 */
f32 srgb_to_linear(f32 v)
{
	if (v > 0.04045f)
		return powf((v + 0.055f) / 1.055f, 2.4f);
	return v / 12.92f;
}

/*! \brief Convert an 8-bit sRGB colour to floating-point linear YCbCr
 *
 * 0.5 is added to Cb and Cr so that they are in [0;1]
 */
void rgb2ycbcr(u8 r_8, u8 g_8, u8 b_8, f32 &y, f32 &cb, f32 &cr)
{
	f32 r = srgb_to_linear(r_8 / 255.0f);
	f32 g = srgb_to_linear(g_8 / 255.0f);
	f32 b = srgb_to_linear(b_8 / 255.0f);
	y = (0.299f * r + 0.587f * g + 0.114f * b);
	cb = (-0.168736f * r - 0.331264f * g + 0.5f * b) + 0.5f;
	cr = (0.5f * r - 0.418688f * g - 0.081312f * b) + 0.5f;
}

/*! \brief The inverse of rgb2ycbcr
 *
 * The coefficents are from http://www.equasys.de/colorconversion.html.
 * If RGB values are too big or small, they are clamped.
 */
void ycbcr2rgb(f32 y, f32 cb, f32 cr, u8 &r_8, u8 &g_8, u8 &b_8)
{
	f32 r = (y + 1.402f * (cr - 0.5f));
	f32 g = (y - 0.344136f * (cb - 0.5f) - 0.714136f * (cr - 0.5f));
	f32 b = (y + 1.772f * (cb - 0.5f));
	r = linear_to_srgb(r);
	g = linear_to_srgb(g);
	b = linear_to_srgb(b);
	r_8 = CLAMP(r * 255.0f, 0, 255);
	g_8 = CLAMP(g * 255.0f, 0, 255);
	b_8 = CLAMP(b * 255.0f, 0, 255);
}

/*! \brief Convert a BGRA image to four floating-point matrices
 *
 * \param raw The 8-bit sRGB BGRA input image data
 * \param matrices The output matrices for the Y, Cb, Cr and alpha channels
 */
void image_to_matrices(const u32 *raw, std::array<Matrix, 4> &matrices)
{
	u32 w{matrices[0].w};
	u32 h{matrices[0].h};
	for (u32 i = 0; i < w * h; ++i) {
		const u8 *bgra{reinterpret_cast<const u8 *>(&raw[i])};
		rgb2ycbcr(*(bgra+2), *(bgra+1), *bgra,
			matrices[0].data[i], matrices[1].data[i], matrices[2].data[i]);
		matrices[3].data[i] = *(bgra+3) / 255.0f;
	}
}

/*! \brief The inverse of image_to_matrices
 */
void matrices_to_image(const std::array<Matrix, 4> &matrices, u32 *raw)
{
	u32 w{matrices[0].w};
	u32 h{matrices[0].h};
	for (u32 i{0}; i < w * h; ++i) {
		u8 *bgra{reinterpret_cast<u8 *>(&raw[i])};
		ycbcr2rgb(matrices[0].data[i], matrices[1].data[i], matrices[2].data[i],
			*(bgra+2), *(bgra+1), *bgra);
		f32 alpha{matrices[3].data[i] * 255};
		*(bgra+3) = CLAMP(alpha, 0, 255);
	}
}

/*! \brief Downscale the input and squared input to multiple lower resolutions
 *
 * \param mat The input data
 * \param targets The resolutions and memory for the lower-resolution output
 */
void downscale(const Matrix &mat,
	const std::vector<std::array<Matrix, 2>> &targets)
{
	u32 w{mat.w};
	u32 h{mat.h};
	u32 input_size{w * h};
	const f32 *l{mat.data.get()};
	std::unique_ptr<f32[]> l2_init{new f32[input_size]};
	f32 *l2{l2_init.get()};
	for (u32 i{0}; i < input_size; ++i) {
		l2[i] = l[i] * l[i];
	}
	for (const auto &mats_smaller : targets) {
		const Matrix &mat_smaller_l{mats_smaller[0]};
		const Matrix &mat_smaller_l2{mats_smaller[1]};
		u32 w2{mat_smaller_l.w};
		u32 h2{mat_smaller_l.h};
		u32 scaling_w{w / w2};
		u32 scaling_h{h / h2};
		f32 divider_s = 1.0f / (scaling_w * scaling_h);
		for (u32 y_small{0}; y_small < h2; ++y_small) {
			for (u32 x_small{0}; x_small < w2; ++x_small) {
				f32 acc_l{0};
				f32 acc_l2{0};
				u32 x{x_small * scaling_w};
				u32 y{y_small * scaling_h};
				for (u32 yc = y; yc < y + scaling_h; ++yc) {
					for (u32 xc = x; xc < x + scaling_w; ++xc) {
						u32 vi{INDEX(xc % w, yc % h, w)};
						acc_l += l[vi];
						acc_l2 += l2[vi];
					}
				}
				u32 vi{INDEX(x_small, y_small, w2)};
				mat_smaller_l.data[vi] = acc_l * divider_s;
				mat_smaller_l2.data[vi] = acc_l2 * divider_s;
			}
		}
		w = w2;
		h = h2;
		l = mat_smaller_l.data.get();
		l2 = mat_smaller_l2.data.get();
	}
}

/*! \brief Calculate the perceptually-downscaled output using the downscaled
 * input (L) and downscaled squared input (L2)
 *
 * \param mats The input data L and L2
 * \param target The perceptually-downscaled output
 */
void sharpen(const std::array<Matrix, 2> &mats, Matrix &target)
{
	u32 w{mats[0].w};
	u32 h{mats[0].h};
	const auto &l{mats[0].data};
	const auto &l2{mats[1].data};
	std::unique_ptr<f32[]> m_all{new f32[w * h]};
	std::unique_ptr<f32[]> r_all{new f32[w * h]};
	auto &d{target.data};

	f32 patch_sz_div = 1.0f / (SQR_NP * SQR_NP);

	// Calculate m and r for all patch offsets
	for (u32 y_start{0}; y_start < h; ++y_start) {
		for (u32 x_start{0}; x_start < w; ++x_start) {
			f32 acc_m{0};
			f32 acc_r_1{0};
			f32 acc_r_2{0};
			for (u32 y{y_start}; y < y_start + SQR_NP; ++y) {
				for (u32 x{x_start}; x < x_start + SQR_NP; ++x) {
					u32 i{INDEX(x % w, y % h, w)};
					acc_m += l[i];
					acc_r_1 += l[i] * l[i];
					acc_r_2 += l2[i];
				}
			}
			f32 mv{acc_m * patch_sz_div};
			f32 slv{acc_r_1 * patch_sz_div - mv * mv};
			f32 shv{acc_r_2 * patch_sz_div - mv * mv};
			u32 i{INDEX(x_start, y_start, w)};
			m_all[i] = mv;
			if (slv >= 0.000001f) // epsilon is 10⁻⁶
				r_all[i] = sqrtf(shv / slv);
			else
				r_all[i] = 1.0f;
		}
	}

	// Calculate the average of the results of all possible patch sets
	// d is the output
	for (u32 y{0}; y < h; ++y) {
		for (u32 x{0}; x < w; ++x) {
			u32 i{INDEX(x, y, w)};
			f32 liner_scaled{l[i]};
			f32 acc_d{0};
			for (s32 y_offset{0}; y_offset > -SQR_NP; --y_offset) {
				for (s32 x_offset{0}; x_offset > -SQR_NP; --x_offset) {
					s32 x_patch_off{static_cast<s32>(x) + x_offset};
					s32 y_patch_off{static_cast<s32>(y) + y_offset};
					x_patch_off = (x_patch_off + w) % w;
					y_patch_off = (y_patch_off + h) % h;
					u32 i_patch_off{INDEX(x_patch_off, y_patch_off, w)};
					f32 mv{m_all[i_patch_off]};
					f32 rv{r_all[i_patch_off]};
					acc_d += mv + rv * liner_scaled - rv * mv;
				}
			}
			d[i] = liner_scaled * LINEAR_RATIO
				+ acc_d * patch_sz_div * (1.0f - LINEAR_RATIO);
		}
	}
}

/*! \brief Downscale the input and save the result to the mip map texture data
 *
 * The downscaling algorithm is based on "Perceptually Based Downscaling of
 * Images" by A. Cengiz Öztireli and Markus Gross.
 *
 * \param matrices The image channels from the high-resolution texture
 * \param target_resolutions_perc A list of target resolutions and corresponding
 *   memory locations for the final BGRA output
 */
void downscale_images(const std::array<Matrix, 4> &matrices,
	const std::vector<std::pair<std::array<u32, 2>, u32*>>
	&target_resolutions_perc)
{
	std::array<std::vector<Matrix>, 4> results;
	for (u8 channel{0}; channel < 4; ++channel) {
		// Allocate the matrices
		std::vector<std::array<Matrix, 2>> downscaleds;
		for (const auto &res : target_resolutions_perc) {
			u32 w{res.first[0]};
			u32 h{res.first[1]};
			downscaleds.emplace_back(std::array<Matrix, 2>{Matrix(w, h),
				Matrix(w, h)});
			results[channel].emplace_back(w, h);
		}
		// Perform the downscaling
		downscale(matrices[channel], downscaleds);
		for (u32 i{0}; i < target_resolutions_perc.size(); ++i) {
			sharpen(downscaleds[i], results[channel][i]);
		}
	}
	// Convert the result back to 8-bit colours
	for (u32 i{0}; i < target_resolutions_perc.size(); ++i) {
		std::array<Matrix, 4> smaller_matrices{std::move(results[0][i]),
			std::move(results[1][i]), std::move(results[2][i]),
			std::move(results[3][i])};
		matrices_to_image(smaller_matrices, target_resolutions_perc[i].second);
	}
}

/*! \brief Function for linearly downscaling a stripe
 *
 * \param parent_stripe Pointer to the BGRA data of the parent texture
 * \param parent_stripe_len Size of the parent texture, whose resolution is
 *   either 1 x parent_stripe_len or parent_stripe_len x 1 since it is a stripe
 * \param target_stripe Pointer to BGRA data where the downscaled stripe is
 *   saved
 */
void downscale_stripe(const u32 *parent_stripe, u32 parent_stripe_len,
	u32 *target_stripe)
{
	// If parent_stripe_len is odd, we ignore the last pixel in the parent
	// stripe.
	for (u32 x{0}; x < parent_stripe_len / 2; ++x) {
		const u8 *bgra1{reinterpret_cast<const u8 *>(parent_stripe + 2 * x)};
		const u8 *bgra2{reinterpret_cast<const u8 *>(
			parent_stripe + 2 * x + 1)};
		u8 *bgra_target{reinterpret_cast<u8 *>(target_stripe + x)};
		// Average the colours from the two pixels in the linearised sRGB colour
		// space
		for (u8 i{0}; i < 3; ++i) {
			f32 v1{srgb_to_linear(bgra1[i] / 255.0f)};
			f32 v2{srgb_to_linear(bgra2[i] / 255.0f)};
			bgra_target[i] = linear_to_srgb(0.5f * (v1 + v2)) * 255.0f;
		}
		// Average the alpha value from the left and right pixel
		bgra_target[3] = (static_cast<u16>(bgra1[3]) + bgra2[3]) / 2;
	}
}

}  // namespace


void generate_custom_mipmaps(video::IImage &img)
{
	core::dimension2d<u32> dim{img.getDimension()};
	u32 w{dim.Width};
	u32 h{dim.Height};

	if (img.getColorFormat() != video::ECF_A8R8G8B8
			|| img.getImageDataSizeInBytes() != w * h * 4) {
		throw std::runtime_error(
			"The mipmap generation requires a video::ECF_A8R8G8B8 image.");
	}
	// The bytes are in bgra order (in big endian order)

	// Get the number of mip map images and their total size in bytes.
	// Mip maps are generated until the width and height are 1,
	// see https://git.io/vNgmX
	u32 total_pixel_cnt{0};
	u8 mipmapcnt{0};
	while (w > 1 || h > 1) {
		w = MAX(w / 2, 1);
		h = MAX(h / 2, 1);
		total_pixel_cnt += w * h;
		++mipmapcnt;
	}
	std::unique_ptr<u32[]> mip_maps_data{new u32[total_pixel_cnt]};

	w = dim.Width;
	h = dim.Height;
	std::array<Matrix, 4> matrices{Matrix(w, h), Matrix(w, h), Matrix(w, h),
		Matrix(w, h)};
	image_to_matrices(static_cast<u32 *>(img.getData()), matrices);

	// Collect target resolutons and associated memory locations
	std::vector<std::pair<std::array<u32, 2>, u32*>> target_resolutions_perc;
	u8 k;
	u32 *current_target{mip_maps_data.get()};
	for (k = 0; k < mipmapcnt; ++k) {
		if (w == 1 || h == 1) {
			// Stripes are downscaled differently
			break;
		}
		// Each step the size is halved and floored
		w /= 2;
		h /= 2;
		target_resolutions_perc.emplace_back(std::array<u32, 2>{w, h},
			current_target);
		// Make current_target point to the next smaller image
		current_target += w * h;
	}

	// Calculate mip maps (except for stripes)
	downscale_images(matrices, target_resolutions_perc);

	// Calculate mip maps for stripes (only if the input image is not a square)
	const u32 *previous_stripe{current_target - w * h};
	if (k == 0)
		previous_stripe = static_cast<u32 *>(img.getData());
	bool horizontal_stripe{h == 1};
	for (; k < mipmapcnt; ++k) {
		u32 parent_stripe_len{w * h};
		downscale_stripe(previous_stripe, parent_stripe_len, current_target);
		previous_stripe = current_target;
		if (horizontal_stripe)
			w /= 2;
		else
			h /= 2;
		current_target += w * h;
	}

	// Pass the mip maps to the Irrlicht Image (Irrlicht copies mip_maps_data)
	img.setMipMapsData(mip_maps_data.get(), false);
}

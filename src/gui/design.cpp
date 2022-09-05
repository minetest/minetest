/*
Minetest
Copyright (C) 2022 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include "design.h"

#include "manager.h"

#include <cmath>
#include <vector>

namespace ui
{
	void Design::drawTo(Canvas &canvas, const Rect<float> &dest_rect)
	{
		// Draw the background color if it's not totally transparent.
		if (m_color.getAlpha() != 0x0) {
			canvas.drawRect(dest_rect, m_color);
		}

		// If there is no texture to draw, nothing else need be done.
		if (m_texture.isScreen()) {
			return;
		}

		// If we have animations, we need to adjust the source rect by the frame
		// offset in accordance with the current frame.
		Rect<float> norm_src_rect = m_src_rect;

		if (m_frame_time != 0) {
			s32 frame_idx = (g_manager.getTime() / m_frame_time) % m_num_frames;
			norm_src_rect += m_frame_offset * frame_idx;
		}

		// Convert the normalized coordinates for the source rect to pixels.
		Dim<s32> tex_size = m_texture.getSize();

		Rect<s32> src_rect(
			lround(left(norm_src_rect) * tex_size.Width),
			lround(top(norm_src_rect) * tex_size.Height),
			lround(right(norm_src_rect) * tex_size.Width),
			lround(bottom(norm_src_rect) * tex_size.Height)
		);

		// Calculate the size of the rect to draw the texture in. This is just
		// the destination rect if we aren't scaling, and the scaled source rect
		// if we are.
		Dim<float> draw_size(
			(m_scale.X == 0.0f) ? dest_rect.getWidth() : src_rect.getWidth() * m_scale.X,
			(m_scale.Y == 0.0f) ? dest_rect.getHeight() : src_rect.getHeight() * m_scale.Y
		);

		if (m_tile_mode[0] != TileMode::NONE) {
			// At the current texture scale, compute the number of tiles we'll
			// need to fully fill the destination rect.
			float num_tiles = dest_rect.getWidth() / draw_size.Width;

			if (m_tile_mode[0] == TileMode::REPEAT) {
				/* If the texture just repeats, we ceil to get an integer number
				 * of tiles to draw. The drawing size is increased by this
				 * factor, meaning that it may be bigger than the destination
				 * rect, as expected.
				 */
				num_tiles = ceil(num_tiles);
				draw_size.Width *= num_tiles;
			} else {
				/* If the texture rounds, round the number of tiles to the
				 * nearest integer to get the best fit. Set the drawing size to
				 * the destination rect size plus space for one extra tile (see
				 * below) since the tiled textures must fit perfectly.
				 */
				num_tiles = round(num_tiles);
				draw_size.Width = dest_rect.getWidth();
			}

			/* Add the number of extra tiles needed to the end of the existing
			 * source rect. This takes advantage of how Irrlicht automatically
			 * wraps textures if their source rects are larger than the texture
			 * bounds, which is much cleaner and almost certainly more
			 * performant than drawing each tile in a loop. The only downside is
			 * that it will wrap the entire image, not just the portion given by
			 * the source rect, but this is good enough for most use-cases.
			 */
			right(src_rect) += src_rect.getWidth() * (num_tiles - 1);
		}

		if (m_tile_mode[1] != TileMode::NONE) {
			// Tiling in the vertical direction behaves in the same manner as
			// in the horizontal direction.
			float num_tiles = dest_rect.getHeight() / draw_size.Height;

			if (m_tile_mode[1] == TileMode::REPEAT) {
				num_tiles = ceil(num_tiles);
				draw_size.Height *= num_tiles;
			} else {
				num_tiles = round(num_tiles);
				draw_size.Height = dest_rect.getHeight();
			}

			bottom(src_rect) += src_rect.getHeight() * (num_tiles - 1);
		}

		// Finally, we get the drawing position by just placing the drawing rect
		// at the proper alignment within the destination rect.
		Vec<float> draw_pos(
			(dest_rect.getWidth() - draw_size.Width) * m_align.X + left(dest_rect),
			(dest_rect.getHeight() - draw_size.Height) * m_align.Y + top(dest_rect)
		);

		// And huzzah, we draw the texture!
		Rect<float> draw_rect(draw_pos, draw_size);
		canvas.drawTexture(draw_rect, m_texture, &src_rect, &dest_rect, m_tint);
	}

	void Design::resetView()
	{
		m_color = BLANK;

		m_texture = Texture();
		m_src_rect = Rect<float>(0.0f, 0.0f, 1.0f, 1.0f);
		m_tint = WHITE;

		m_num_frames = 1;
		m_frame_time = 0;
		m_frame_offset = Vec<float>();

		m_align = Vec<float>();
		m_scale = Vec<float>();

		m_tile_mode = {TileMode::NONE, TileMode::NONE};
	}

	void Design::applyView(const JsonReader &json)
	{
		json["color"].readColor(m_color);

		if (auto val = json["texture"]) {
			m_texture = Texture(g_manager.getTexture(val.toString()));
		}
		json["src_rect"].readRect(m_src_rect);
		json["tint"].readColor(m_tint);

		json["num_frames"].readNum(m_num_frames, 1);
		json["frame_time"].readNum(m_frame_time);
		json["frame_offset"].readVec(m_frame_offset);

		json["align"].readVec(m_align, {0.0f, 0.0f}, {1.0f, 1.0f});
		json["scale"].readVec(m_scale, {0.0f, 0.0f});

		json["tile_mode"].readArray(m_tile_mode,
				JSON_BIND_READ(readEnum<TileMode>, TILE_MODE_NAME_MAP));
	}
}

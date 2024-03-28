/*
Minetest
Copyright (C) 2023 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include "gui/box.h"

#include "debug.h"
#include "log.h"
#include "porting.h"
#include "gui/elem.h"
#include "gui/manager.h"
#include "gui/window.h"
#include "util/serialize.h"

namespace ui
{
	Align toAlign(u8 align)
	{
		if (align >= (u8)Align::MAX_ALIGN) {
			return Align::CENTER;
		}
		return (Align)align;
	}

	void Layer::reset()
	{
		image = Texture();
		fill = BLANK;
		tint = WHITE;

		source = rf32(0.0f, 0.0f, 1.0f, 1.0f);
		middle = rf32(0.0f, 0.0f, 0.0f, 0.0f);
		middle_scale = 1.0f;

		num_frames = 1;
		frame_time = 1000;
	}

	void Layer::read(std::istream &full_is)
	{
		auto is = newIs(readStr16(full_is));
		u32 set_mask = readU32(is);

		if (testShift(set_mask))
			image = g_manager.getTexture(readNullStr(is));
		if (testShift(set_mask))
			fill = readARGB8(is);
		if (testShift(set_mask))
			tint = readARGB8(is);

		if (testShift(set_mask)) {
			source.UpperLeftCorner = readV2F32(is);
			source.LowerRightCorner = readV2F32(is);
		}
		if (testShift(set_mask)) {
			middle.UpperLeftCorner = clamp_vec(readV2F32(is));
			middle.LowerRightCorner = clamp_vec(readV2F32(is));
		}
		if (testShift(set_mask))
			middle_scale = std::max(readF32(is), 0.0f);

		if (testShift(set_mask))
			num_frames = std::max(readU32(is), 1U);
		if (testShift(set_mask))
			frame_time = std::max(readU32(is), 1U);
	}

	void Style::reset()
	{
		size = d2f32(0.0f, 0.0f);

		rel_pos = v2f32(0.0f, 0.0f);
		rel_anchor = v2f32(0.0f, 0.0f);
		rel_size = d2s32(1.0f, 1.0f);

		margin = rf32(0.0f, 0.0f, 0.0f, 0.0f);
		padding = rf32(0.0f, 0.0f, 0.0f, 0.0f);

		bg.reset();
		fg.reset();

		fg_scale = 1.0f;
		fg_halign = Align::CENTER;
		fg_valign = Align::CENTER;

		visible = true;
		noclip = false;
	}

	void Style::read(std::istream &is)
	{
		// No need to read a size prefix; styles are already read in as size-
		// prefixed strings in Window.
		u32 set_mask = readU32(is);

		if (testShift(set_mask))
			size = clamp_vec(readV2F32(is));

		if (testShift(set_mask))
			rel_pos = readV2F32(is);
		if (testShift(set_mask))
			rel_anchor = readV2F32(is);
		if (testShift(set_mask))
			rel_size = clamp_vec(readV2F32(is));

		if (testShift(set_mask)) {
			margin.UpperLeftCorner = readV2F32(is);
			margin.LowerRightCorner = readV2F32(is);
		}
		if (testShift(set_mask)) {
			padding.UpperLeftCorner = readV2F32(is);
			padding.LowerRightCorner = readV2F32(is);
		}

		if (testShift(set_mask))
			bg.read(is);
		if (testShift(set_mask))
			fg.read(is);

		if (testShift(set_mask))
			fg_scale = std::max(readF32(is), 0.0f);
		if (testShift(set_mask))
			fg_halign = toAlign(readU8(is));
		if (testShift(set_mask))
			fg_valign = toAlign(readU8(is));

		if (testShift(set_mask))
			visible = testShift(set_mask);
		if (testShift(set_mask))
			noclip = testShift(set_mask);
	}

	Window &Box::getWindow()
	{
		return m_elem.getWindow();
	}

	const Window &Box::getWindow() const
	{
		return m_elem.getWindow();
	}

	void Box::reset()
	{
		m_style.reset();

		for (State i = 0; i < m_style_refs.size(); i++) {
			m_style_refs[i] = NO_STYLE;
		}

		m_draw_rect = rf32(0.0f, 0.0f, 0.0f, 0.0f);
		m_child_rect = m_draw_rect;
		m_clip_rect = m_draw_rect;
	}

	void Box::read(std::istream &full_is)
	{
		auto is = newIs(readStr16(full_is));
		u32 style_mask = readU32(is);

		for (State i = 0; i < m_style_refs.size(); i++) {
			// If we have a style for this state in the mask, add it to the
			// list of styles.
			if (!testShift(style_mask)) {
				continue;
			}

			u32 style = readU32(is);
			if (getWindow().getStyleStr(style) != nullptr) {
				m_style_refs[i] = style;
			} else {
				errorstream << "Style " << style << " does not exist" << std::endl;
			}
		}
	}

	void Box::layout(const rf32 &parent_rect, const rf32 &parent_clip)
	{
		// Before we layout the box, we need to recompute the style so we have
		// fully updated style properties.
		computeStyle();

		// First, calculate the size of the box in absolute coordinates based
		// on the normalized size.
		d2f32 origin_size(
			(m_style.rel_size.Width  * parent_rect.getWidth()),
			(m_style.rel_size.Height * parent_rect.getHeight())
		);

		// Ensure that the normalized size of the box isn't smaller than the
		// minimum size.
		origin_size.Width  = std::max(origin_size.Width,  m_style.size.Width);
		origin_size.Height = std::max(origin_size.Height, m_style.size.Height);

		// Then, create the rect of the box relative to the origin by
		// converting the normalized position absolute coordinates, while
		// accounting for the anchor based on the previously calculated size.
		v2f32 origin_pos(
			(m_style.rel_pos.X * parent_rect.getWidth()) -
					(m_style.rel_anchor.X * origin_size.Width),
			(m_style.rel_pos.Y * parent_rect.getHeight()) -
					(m_style.rel_anchor.Y * origin_size.Height)
		);

		rf32 origin_rect(origin_pos, origin_size);

		// The absolute rect of the box is made by shifting the origin to the
		// top left of the parent rect.
		rf32 abs_rect = origin_rect + parent_rect.UpperLeftCorner;

		// The rect we draw to is the absolute rect adjusted for the margins.
		// Since this is the final rect, we ensure that it doesn't have a
		// negative size.
		m_draw_rect = clamp_rect(rf32(
			abs_rect.UpperLeftCorner  + m_style.margin.UpperLeftCorner,
			abs_rect.LowerRightCorner - m_style.margin.LowerRightCorner
		));

		// The rect that children and the foreground layer are drawn relative
		// to is the draw rect adjusted for padding. Make sure this rect is
		// never negative as well.
		m_child_rect = clamp_rect(rf32(
			m_draw_rect.UpperLeftCorner  + m_style.padding.UpperLeftCorner,
			m_draw_rect.LowerRightCorner - m_style.padding.LowerRightCorner
		));

		// If we are set to noclip, we clip to the same rect we draw to.
		// Otherwise, the clip rect is the drawing rect clipped against the
		// parent clip rect.
		m_clip_rect = m_style.noclip ? m_draw_rect : clip_rect(m_draw_rect, parent_clip);
	}

	void Box::draw(Canvas &parent)
	{
		// Since layout() is always called before draw(), we already have fully
		// updated style properties.

		// Don't draw anything if we aren't visible.
		if (!m_style.visible) {
			return;
		}

		// Create a new canvas relative to our parent to draw to.
		Canvas canvas(parent, m_style.noclip ? nullptr : &m_clip_rect);

		// Draw our background and foreground layers.
		drawLayer(canvas, m_style.bg, m_draw_rect);
		drawForeground(canvas);
	}

	void Box::drawForeground(Canvas &canvas)
	{
		// It makes no sense to draw a foreground when there's no image, since
		// it would otherwise take no room.
		if (!m_style.fg.image.isTexture()) {
			return;
		}

		// The foreground layer is aligned and scaled in a particular area of
		// the box. First, get the size of the foreground layer.
		d2f32 src_size = m_style.fg.source.getSize();
		src_size.Height /= m_style.fg.num_frames;

		d2s32 tex_size = m_style.fg.image.getSize();
		src_size.Width *= tex_size.Width;
		src_size.Height *= tex_size.Height;

		// Then, compute the scale that we should use. A scale of zero means
		// the image should take up as much room as possible while still
		// preserving the aspect ratio of the image.
		float scale = m_style.fg_scale;

		if (scale == 0.0f) {
			scale = std::min(
				m_child_rect.getWidth()  / src_size.Width,
				m_child_rect.getHeight() / src_size.Height
			);
		}

		d2f32 fg_size(src_size.Width * scale, src_size.Height * scale);

		// Now, using the alignment options, position the foreground image
		// inside the remaining space.
		v2f32 fg_pos = m_child_rect.UpperLeftCorner;

		if (m_style.fg_halign == Align::CENTER) {
			fg_pos.X += (m_child_rect.getWidth() - fg_size.Width) / 2.0f;
		} else if (m_style.fg_halign == Align::END) {
			fg_pos.X += m_child_rect.getWidth() - fg_size.Width;
		}

		if (m_style.fg_valign == Align::CENTER) {
			fg_pos.Y += (m_child_rect.getHeight() - fg_size.Height) / 2.0f;
		} else if (m_style.fg_valign == Align::END) {
			fg_pos.Y += m_child_rect.getHeight() - fg_size.Height;
		}

		// We have our position and size, so now we can draw the layer.
		drawLayer(canvas, m_style.fg, rf32(fg_pos, fg_size));
	}

	void Box::drawLayer(Canvas &canvas, const Layer &layer, const rf32 &dst)
	{
		// Draw the fill color if it's not totally transparent.
		if (layer.fill.getAlpha() != 0x0) {
			canvas.drawRect(dst, layer.fill);
		}

		// If there's no image, there's nothing else for us to do.
		if (!layer.image.isTexture()) {
			return;
		}

		// If we have animations, we need to adjust the source rect by the
		// frame offset in accordance with the current frame.
		rf32 src = layer.source;

		if (layer.num_frames > 1) {
			float frame_height = src.getHeight() / layer.num_frames;
			src.LowerRightCorner.Y = src.UpperLeftCorner.Y + frame_height;

			float frame_offset = frame_height *
				((porting::getTimeMs() / layer.frame_time) % layer.num_frames);
			src.UpperLeftCorner.Y  += frame_offset;
			src.LowerRightCorner.Y += frame_offset;
		}

		// If the source rect for this image is flipped, we need to flip the
		// sign of our middle rect as well to get the right adjustments.
		rf32 src_middle = layer.middle;

		if (src.getWidth() < 0.0f) {
			src_middle.UpperLeftCorner.X  = -src_middle.UpperLeftCorner.X;
			src_middle.LowerRightCorner.X = -src_middle.LowerRightCorner.X;
		}
		if (src.getHeight() < 0.0f) {
			src_middle.UpperLeftCorner.Y  = -src_middle.UpperLeftCorner.Y;
			src_middle.LowerRightCorner.Y = -src_middle.LowerRightCorner.Y;
		}

		// Now we need to draw the texture as a nine-slice image. But first,
		// since the middle rect uses normalized coordinates, we need to
		// de-normalize it into actual pixels for the destination rect and
		// scale it by the middle rect scaling parameter.
		rf32 scaled_middle(
			layer.middle.UpperLeftCorner.X  * layer.middle_scale * layer.image.getWidth(),
			layer.middle.UpperLeftCorner.Y  * layer.middle_scale * layer.image.getHeight(),
			layer.middle.LowerRightCorner.X * layer.middle_scale * layer.image.getWidth(),
			layer.middle.LowerRightCorner.Y * layer.middle_scale * layer.image.getHeight()
		);

		// Now draw each slice of the nine-slice image. If the middle rect
		// equals the whole source rect, this will automatically act like a
		// normal image.
		for (int y = 0; y < 3; y++) {
			for (int x = 0; x < 3; x++) {
				rf32 slice_src = src;
				rf32 slice_dst = dst;

				switch (x) {
				case 0:
					slice_dst.LowerRightCorner.X =
						dst.UpperLeftCorner.X + scaled_middle.UpperLeftCorner.X;
					slice_src.LowerRightCorner.X =
						src.UpperLeftCorner.X + src_middle.UpperLeftCorner.X;
					break;

				case 1:
					slice_dst.UpperLeftCorner.X += scaled_middle.UpperLeftCorner.X;
					slice_dst.LowerRightCorner.X -= scaled_middle.LowerRightCorner.X;
					slice_src.UpperLeftCorner.X += src_middle.UpperLeftCorner.X;
					slice_src.LowerRightCorner.X -= src_middle.LowerRightCorner.X;
					break;

				case 2:
					slice_dst.UpperLeftCorner.X =
						dst.LowerRightCorner.X - scaled_middle.LowerRightCorner.X;
					slice_src.UpperLeftCorner.X =
						src.LowerRightCorner.X - src_middle.LowerRightCorner.X;
					break;
				}

				switch (y) {
				case 0:
					slice_dst.LowerRightCorner.Y =
						dst.UpperLeftCorner.Y + scaled_middle.UpperLeftCorner.Y;
					slice_src.LowerRightCorner.Y =
						src.UpperLeftCorner.Y + src_middle.UpperLeftCorner.Y;
					break;

				case 1:
					slice_dst.UpperLeftCorner.Y += scaled_middle.UpperLeftCorner.Y;
					slice_dst.LowerRightCorner.Y -= scaled_middle.LowerRightCorner.Y;
					slice_src.UpperLeftCorner.Y += src_middle.UpperLeftCorner.Y;
					slice_src.LowerRightCorner.Y -= src_middle.LowerRightCorner.Y;
					break;

				case 2:
					slice_dst.UpperLeftCorner.Y =
						dst.LowerRightCorner.Y - scaled_middle.LowerRightCorner.Y;
					slice_src.UpperLeftCorner.Y =
						src.LowerRightCorner.Y - src_middle.LowerRightCorner.Y;
					break;
				}

				// Draw this slice of the texture with the proper tint.
				canvas.drawTexture(slice_dst, layer.image, slice_src, layer.tint);
			}
		}
	}

	void Box::computeStyle()
	{
		// First, clear our current style and compute what state we're in.
		m_style.reset();
		State state = STATE_NONE;

		// Loop over each style state from lowest precedence to highest since
		// they should be applied in that order.
		for (State i = 0; i < m_style_refs.size(); i++) {
			// If this state we're looking at is a subset of the current state,
			// then it's a match for styling.
			if ((state & i) != i) {
				continue;
			}

			u32 index = m_style_refs[i];

			// If the index for this state has an associated style string,
			// apply it to our current style.
			if (index != NO_STYLE) {
				auto is = newIs(*getWindow().getStyleStr(index));
				m_style.read(is);
			}
		}
	}
}

// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2023 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

#include "ui/box.h"

#include "debug.h"
#include "log.h"
#include "porting.h"
#include "ui/elem.h"
#include "ui/manager.h"
#include "ui/window.h"
#include "util/serialize.h"

#include <SDL2/SDL.h>

namespace ui
{
	BoxTile toBoxTile(u8 tile)
	{
		if (tile > (u8)BoxTile::MAX_TILE) {
			return BoxTile::NONE;
		}
		return (BoxTile)tile;
	}

	IconPlace toIconPlace(u8 place)
	{
		if (place > (u8)IconPlace::MAX_PLACE) {
			return IconPlace::CENTER;
		}
		return (IconPlace)place;
	}

	void Layer::reset()
	{
		image = nullptr;
		fill = BLANK;
		tint = WHITE;

		scale = 1.0f;
		source = RectF(0.0f, 0.0f, 1.0f, 1.0f);

		num_frames = 1;
		frame_time = 1000;
	}

	void Layer::read(std::istream &full_is)
	{
		auto is = newIs(readStr16(full_is));
		u32 set_mask = readU32(is);

		if (testShift(set_mask))
			image = g_manager.getTexture(readStr16(is));
		if (testShift(set_mask))
			fill = readARGB8(is);
		if (testShift(set_mask))
			tint = readARGB8(is);

		if (testShift(set_mask))
			scale = std::max(readF32(is), 0.0f);
		if (testShift(set_mask))
			source = readRectF(is);

		if (testShift(set_mask))
			num_frames = std::max(readU32(is), 1U);
		if (testShift(set_mask))
			frame_time = std::max(readU32(is), 1U);
	}

	void Style::reset()
	{
		size = SizeF(0.0f, 0.0f);

		rel_pos = PosF(0.0f, 0.0f);
		rel_anchor = PosF(0.0f, 0.0f);
		rel_size = SizeF(1.0f, 1.0f);

		margin = DispF(0.0f, 0.0f, 0.0f, 0.0f);
		padding = DispF(0.0f, 0.0f, 0.0f, 0.0f);

		box.reset();
		icon.reset();

		box_middle = DispF(0.0f, 0.0f, 0.0f, 0.0f);
		box_tile = BoxTile::NONE;

		icon_place = IconPlace::CENTER;
		icon_gutter = 0.0f;
		icon_overlap = false;

		visible = true;
		noclip = false;
	}

	void Style::read(std::istream &is)
	{
		// No need to read a size prefix; styles are already read in as size-
		// prefixed strings in Window.
		u32 set_mask = readU32(is);

		if (testShift(set_mask))
			size = readSizeF(is).clip();

		if (testShift(set_mask))
			rel_pos = readPosF(is);
		if (testShift(set_mask))
			rel_anchor = readPosF(is);
		if (testShift(set_mask))
			rel_size = readSizeF(is).clip();

		if (testShift(set_mask))
			margin = readDispF(is);
		if (testShift(set_mask))
			padding = readDispF(is);

		if (testShift(set_mask))
			box.read(is);
		if (testShift(set_mask))
			icon.read(is);

		if (testShift(set_mask))
			box_middle = readDispF(is).clip();
		if (testShift(set_mask))
			box_tile = toBoxTile(readU8(is));

		if (testShift(set_mask))
			icon_place = toIconPlace(readU8(is));
		if (testShift(set_mask))
			icon_gutter = readF32(is);
		testShiftBool(set_mask, icon_overlap);

		testShiftBool(set_mask, visible);
		testShiftBool(set_mask, noclip);
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
		m_content = nullptr;
		m_style.reset();

		for (State i = 0; i < m_style_refs.size(); i++) {
			m_style_refs[i] = NO_STYLE;
		}
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

			u32 index = readU32(is);
			if (getWindow().getStyleStr(index) != nullptr) {
				m_style_refs[i] = index;
			} else {
				errorstream << "Style " << index << " does not exist" << std::endl;
			}
		}
	}

	void Box::restyle()
	{
		// First, clear our current style and compute what state we're in.
		m_style.reset();
		State state = STATE_NONE;

		if (m_elem.isBoxFocused(*this))
			state |= STATE_FOCUSED;
		if (m_elem.isBoxSelected(*this))
			state |= STATE_SELECTED;
		if (m_elem.isBoxHovered(*this))
			state |= STATE_HOVERED;
		if (m_elem.isBoxPressed(*this))
			state |= STATE_PRESSED;
		if (m_elem.isBoxDisabled(*this))
			state |= STATE_DISABLED;

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

		// Finally, make sure to restyle our content, if we have any.
		if (m_content != nullptr) {
			m_content->restyle();
		}
	}

	void Box::relayout(RectF parent_rect, RectF parent_clip)
	{
		// The minimum size of the display rect is the user-specified minimum
		// size, so we adjust this for the margins to get the minimum size of
		// the layout rect. Make sure it doesn't become negative.
		SizeF min_size = (m_style.size + m_style.margin.extents()).clip();

		// Calculate the position and size of the box relative to the origin
		// using normalized coordinates, making sure the size doesn't go below
		// the minimum size.
		SizeF norm_size = (m_style.rel_size * parent_rect.size()).max(min_size);
		SizeF norm_pos = (m_style.rel_pos * parent_rect.size()) -
			(m_style.rel_anchor * norm_size);

		// The layout rect of the box is made by shifting the rect obtained by
		// the normalized coordinates to the top left of the parent rect.
		RectF layout_rect = RectF(parent_rect.TopLeft + norm_pos, norm_size);

		// The display rect is created by insetting the layout rect by the
		// margin. The padding rect is inset from that by the middle rect edges
		// and the padding. These may not have negative sizes.
		m_display_rect = layout_rect.insetBy(m_style.margin).clip();
		RectF padding_rect = m_display_rect.insetBy(
			getMiddleEdges() + m_style.padding).clip();

		// The icon is aligned and scaled in a particular area of the box.
		// First, get the size of the icon rect.
		SizeF icon_size = getLayerSize(m_style.icon);

		// Then, compute the scale that we should use. A scale of zero means
		// the image should take up as much room as possible while still
		// preserving the aspect ratio of the image.
		if (m_style.icon.scale == 0.0f) {
			icon_size *= std::min(padding_rect.W() / icon_size.W,
				padding_rect.H() / icon_size.H);
		} else {
			icon_size *= m_style.icon.scale;
		}

		// Now, calculate the icon rect based on the desired placement.
		PosF icon_start  = padding_rect.TopLeft;
		PosF icon_center = icon_start + (padding_rect.size() - icon_size) / 2.0f;
		PosF icon_end    = icon_start + (padding_rect.size() - icon_size);

		switch (m_style.icon_place) {
		case IconPlace::CENTER:
			m_icon_rect = RectF(icon_center, icon_size);
			break;
		case IconPlace::LEFT:
			m_icon_rect = RectF(PosF(icon_start.X, icon_center.Y), icon_size);
			break;
		case IconPlace::TOP:
			m_icon_rect = RectF(PosF(icon_center.X, icon_start.Y), icon_size);
			break;
		case IconPlace::RIGHT:
			m_icon_rect = RectF(PosF(icon_end.X, icon_center.Y), icon_size);
			break;
		case IconPlace::BOTTOM:
			m_icon_rect = RectF(PosF(icon_center.X, icon_end.Y), icon_size);
			break;
		}

		// If the overlap property is set or the icon is centered, the content
		// rect is identical to the padding rect. Otherwise, the content rect
		// needs to be adjusted to account for the icon and gutter.
		m_content_rect = padding_rect;

		if (!m_style.icon_overlap && m_style.icon.image != nullptr) {
			switch (m_style.icon_place) {
			case IconPlace::CENTER:
				break;
			case IconPlace::LEFT:
				m_content_rect.L += icon_size.W + m_style.icon_gutter;
				break;
			case IconPlace::TOP:
				m_content_rect.T += icon_size.H + m_style.icon_gutter;
				break;
			case IconPlace::RIGHT:
				m_content_rect.R -= icon_size.W + m_style.icon_gutter;
				break;
			case IconPlace::BOTTOM:
				m_content_rect.B -= icon_size.H + m_style.icon_gutter;
				break;
			}
		}

		// If we are set to noclip, we clip to the same rect we draw to.
		// Otherwise, the clip rect is the drawing rect clipped against the
		// parent clip rect.
		m_clip_rect = m_style.noclip ?
			m_display_rect : m_display_rect.intersectWith(parent_clip);

		// Finally, if we have content to layout, do so in the content rect.
		if (m_content != nullptr) {
			m_content->relayout(m_content_rect, m_clip_rect);
		}
	}

	void Box::draw()
	{
		if (m_style.visible) {
			drawBox();
			drawIcon();
		}

		if (m_content != nullptr) {
			m_content->draw();
		}
	}

	bool Box::isPointed() const
	{
		return m_clip_rect.contains(getWindow().getPointerPos()) ||
				(m_content != nullptr && m_content->isPointed());
	}

	bool Box::processInput(const SDL_Event &event)
	{
		switch (event.type) {
		case UI_USER(FOCUS_REQUEST):
			// The box is dynamic, so it can be focused.
			return true;

		case UI_USER(FOCUS_CHANGED):
			// If the box is no longer focused, it can't be pressed.
			if (event.user.data1 == &m_elem) {
				setPressed(false);
			}
			return false;

		case UI_USER(FOCUS_SUBVERTED):
			// If some non-focused element used an event instead of this one,
			// unpress the box because user interaction has been diverted.
			setPressed(false);
			return false;

		case UI_USER(HOVER_REQUEST):
			// The box can be hovered if the pointer is inside it.
			return isPointed();

		case UI_USER(HOVER_CHANGED):
			// Make this box hovered if the element became hovered and the
			// pointer is inside this box.
			setHovered(event.user.data2 == &m_elem && isPointed());
			return true;

		default:
			return false;
		}
	}

	bool Box::processFullPress(const SDL_Event &event, void (*on_press)(Elem &))
	{
		switch (event.type) {
		case SDL_KEYDOWN:
			// If the space key is pressed not due to a key repeat, then the
			// box becomes pressed. If the escape key is pressed while the box
			// is pressed, that unpresses the box without triggering it.
			if (event.key.keysym.sym == SDLK_SPACE && !event.key.repeat) {
				setPressed(true);
				return true;
			} else if (event.key.keysym.sym == SDLK_ESCAPE && isPressed()) {
				setPressed(false);
				return true;
			}
			return false;

		case SDL_KEYUP:
			// Releasing the space key while the box is pressed causes it to be
			// unpressed and triggered.
			if (event.key.keysym.sym == SDLK_SPACE && isPressed()) {
				setPressed(false);
				on_press(m_elem);
				return true;
			}
			return false;

		case SDL_MOUSEBUTTONDOWN:
			// If the box is hovered, then pressing the left mouse button
			// causes it to be pressed. Otherwise, the mouse is directed at
			// some other box.
			if (isHovered() && event.button.button == SDL_BUTTON_LEFT) {
				setPressed(true);
				return true;
			}
			return false;

		case SDL_MOUSEBUTTONUP:
			// If the mouse button was released, the box becomes unpressed. If
			// it was released while inside the bounds of the box, that counts
			// as the box being triggered.
			if (event.button.button == SDL_BUTTON_LEFT) {
				bool was_pressed = isPressed();
				setPressed(false);

				if (isHovered() && was_pressed) {
					on_press(m_elem);
					return true;
				}
			}
			return false;

		default:
			return processInput(event);
		}
	}

	RectF Box::getLayerSource(const Layer &layer)
	{
		RectF src = layer.source;

		// If we have animations, we need to adjust the source rect by the
		// frame offset in accordance with the current frame.
		if (layer.num_frames > 1) {
			float frame_height = src.H() / layer.num_frames;
			src.B = src.T + frame_height;

			float frame_offset = frame_height *
				((porting::getTimeMs() / layer.frame_time) % layer.num_frames);
			src.T += frame_offset;
			src.B += frame_offset;
		}

		return src;
	}

	SizeF Box::getLayerSize(const Layer &layer)
	{
		return getLayerSource(layer).size() * SizeF(getTextureSize(layer.image));
	}

	DispF Box::getMiddleEdges()
	{
		// Scale the middle rect by the scaling factor and de-normalize it into
		// actual pixels based on the image source rect.
		return m_style.box_middle * DispF(getLayerSize(m_style.box)) * m_style.box.scale;
	}

	void Box::drawBox()
	{
		// First, fill the display rectangle with the fill color.
		getWindow().drawRect(m_display_rect, m_clip_rect, m_style.box.fill);

		// If there's no image, then we don't need to do a bunch of
		// calculations in order to draw nothing.
		if (m_style.box.image == nullptr) {
			return;
		}

		// For the image, first get the source rect adjusted for animations.
		RectF src = getLayerSource(m_style.box);

		// We need to make sure the the middle rect is relative to the source
		// rect rather than the entire image, so scale the edges appropriately.
		DispF middle_src = m_style.box_middle * DispF(src.size());
		DispF middle_dst = getMiddleEdges();

		// If the source rect for this image is flipped, we need to flip the
		// sign of our middle rect as well to get the right adjustments.
		if (src.W() < 0.0f) {
			middle_src.L = -middle_src.L;
			middle_src.R = -middle_src.R;
		}
		if (src.H() < 0.0f) {
			middle_src.T = -middle_src.T;
			middle_src.B = -middle_src.B;
		}

		for (int slice_y = 0; slice_y < 3; slice_y++) {
			for (int slice_x = 0; slice_x < 3; slice_x++) {
				// Compute each slice of the nine-slice image. If the middle
				// rect equals the whole source rect, the middle slice will
				// occupy the entire display rectangle.
				RectF slice_src = src;
				RectF slice_dst = m_display_rect;

				switch (slice_x) {
				case 0:
					slice_dst.R = slice_dst.L + middle_dst.L;
					slice_src.R = slice_src.L + middle_src.L;
					break;

				case 1:
					slice_dst.L += middle_dst.L;
					slice_dst.R -= middle_dst.R;
					slice_src.L += middle_src.L;
					slice_src.R -= middle_src.R;
					break;

				case 2:
					slice_dst.L = slice_dst.R - middle_dst.R;
					slice_src.L = slice_src.R - middle_src.R;
					break;
				}

				switch (slice_y) {
				case 0:
					slice_dst.B = slice_dst.T + middle_dst.T;
					slice_src.B = slice_src.T + middle_src.T;
					break;

				case 1:
					slice_dst.T += middle_dst.T;
					slice_dst.B -= middle_dst.B;
					slice_src.T += middle_src.T;
					slice_src.B -= middle_src.B;
					break;

				case 2:
					slice_dst.T = slice_dst.B - middle_dst.B;
					slice_src.T = slice_src.B - middle_src.B;
					break;
				}

				// If we have a tiled image, then some of the tiles may bleed
				// out of the slice rect, so we need to clip to both the
				// clipping rect and the destination rect.
				RectF slice_clip = m_clip_rect.intersectWith(slice_dst);

				// If this slice is empty or has been entirely clipped, then
				// don't bother drawing anything.
				if (slice_clip.empty()) {
					continue;
				}

				// This may be a tiled image, so we need to calculate the size
				// of each tile. If the image is not tiled, this should equal
				// the size of the destination rect.
				SizeF tile_size = slice_dst.size();

				if (m_style.box_tile != BoxTile::NONE) {
					// We need to calculate the tile size based on the texture
					// size and the scale of each tile. If the scale is too
					// small, then the number of tiles will explode, so we
					// clamp it to a reasonable minimum of 1/8 of a pixel.
					SizeF tex_size = getTextureSize(m_style.box.image);
					float tile_scale = std::max(m_style.box.scale, 0.125f);

					if (m_style.box_tile != BoxTile::Y) {
						tile_size.W = slice_src.W() * tex_size.W * tile_scale;
					}
					if (m_style.box_tile != BoxTile::X) {
						tile_size.H = slice_src.H() * tex_size.H * tile_scale;
					}
				}

				// Now we can draw each tile for this slice. If the image is
				// not tiled, then each of these loops will run only once.
				float tile_y = slice_dst.T;

				while (tile_y < slice_dst.B) {
					float tile_x = slice_dst.L;

					while (tile_x < slice_dst.R) {
						// Draw the texture in the appropriate destination rect
						// for this tile, and clip it to the clipping rect for
						// this slice.
						RectF tile_dst = RectF(PosF(tile_x, tile_y), tile_size);

						getWindow().drawTexture(tile_dst, slice_clip,
								m_style.box.image, slice_src, m_style.box.tint);

						tile_x += tile_size.W;
					}
					tile_y += tile_size.H;
				}
			}
		}
	}

	void Box::drawIcon()
	{
		// The icon rect is computed while the box is being laid out, so we
		// just need to draw it with the fill color behind it.
		getWindow().drawRect(m_icon_rect, m_clip_rect, m_style.icon.fill);
		getWindow().drawTexture(m_icon_rect, m_clip_rect, m_style.icon.image,
			getLayerSource(m_style.icon), m_style.icon.tint);
	}

	bool Box::isHovered() const
	{
		return m_elem.getHoveredBox() == getId();
	}

	bool Box::isPressed() const
	{
		return m_elem.getPressedBox() == getId();
	}

	void Box::setHovered(bool hovered)
	{
		if (hovered) {
			m_elem.setHoveredBox(getId());
		} else if (isHovered()) {
			m_elem.setHoveredBox(NO_ID);
		}
	}

	void Box::setPressed(bool pressed)
	{
		if (pressed) {
			m_elem.setPressedBox(getId());
		} else if (isPressed()) {
			m_elem.setPressedBox(NO_ID);
		}
	}
}

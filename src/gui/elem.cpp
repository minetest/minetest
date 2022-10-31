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

#include "elem.h"

#include "env.h"
#include "manager.h"

#include <unordered_map>

namespace ui
{
	void LayoutItem::applyLayout(const JsonReader &json)
	{
		json["elems"].readSet<size_t>(elems,
				JSON_BIND_TO(toNum<size_t>, 0, Lim<size_t>::high()));
		json["weight"].readNum(weight, 0.0f);
	}

	void LayoutAxis::applyLayout(const JsonReader &json)
	{
		json["dir"].readEnum(dir, LAYOUT_DIR_NAME_MAP);

		json["items"].readVector<LayoutItem>(items, [](const JsonReader &val) {
			LayoutItem item;
			item.applyLayout(val);
			return item;
		});

		json["spacing"].readArray(spacing,
				JSON_BIND_READ(readNum<float>, Lim<float>::low(), Lim<float>::high()));

		json["extra_space"].readEnum(extra_space, EXTRA_SPACE_NAME_MAP);
		json["align"].readNum(align);
	}

	void Elem::draw()
	{
		if (!m_visible) {
			return;
		}

		Canvas canvas{Texture()};
		canvas.setScale(m_env.getPixelSize());
		canvas.setSubRect(m_clip_rect);
		canvas.setNoClip(m_noclip);

		onDraw(canvas);

		for (size_t i = 0; i < m_children.size(); i++) {
			m_env.getElem(m_children[i]).draw();
		}
	}

	void Elem::onDraw(Canvas &canvas)
	{
		m_background.drawTo(canvas, m_draw_rect);

		// For clarity and brevity's sake, we refer to the outside of the border
		// as `outer` and the inside as `inner`.
		const Rect<float> &outer = m_draw_rect;
		Rect<float> inner = sub_rect_edges(m_draw_rect, m_border);

		// Irrelevant to functionality, but I love how the length of these lines
		// just happens to increase by one character each time.
		m_slices[SLICE_TOP_LEFT].drawTo(canvas,
				{left(outer), top(outer), left(inner), top(inner)});
		m_slices[SLICE_TOP].drawTo(canvas,
				{left(inner), top(outer), right(inner), top(inner)});
		m_slices[SLICE_TOP_RIGHT].drawTo(canvas,
				{right(inner), top(outer), right(outer), top(inner)});

		m_slices[SLICE_LEFT].drawTo(canvas,
				{left(outer), top(inner), left(inner), bottom(inner)});
		m_slices[SLICE_CENTER].drawTo(canvas,
				{left(inner), top(inner), right(inner), bottom(inner)});
		m_slices[SLICE_RIGHT].drawTo(canvas,
				{right(inner), top(inner), right(outer), bottom(inner)});

		m_slices[SLICE_BOTTOM_LEFT].drawTo(canvas,
				{left(outer), bottom(inner), left(inner), bottom(outer)});
		m_slices[SLICE_BOTTOM].drawTo(canvas,
				{left(inner), bottom(inner), right(inner), bottom(outer)});
		m_slices[SLICE_BOTTOM_RIGHT].drawTo(canvas,
				{right(inner), bottom(inner), right(outer), bottom(outer)});
	}

	void Elem::applyJson(const JsonReader &json)
	{
		if (auto val = json["state"]) {
			applyState(val);
		}
		if (auto val = json["layout"]) {
			applyLayout(val);
		}
		if (auto val = json["view"]) {
			applyView(val);
		}
	}

	void Elem::applyState(const JsonReader &json)
	{
		json["children"].readVector<std::string>(m_children,
				JSON_BIND_TO(toId, MAIN_ID_CHARS, false));
		json["classes"].readSet<std::string>(m_classes,
				JSON_BIND_TO(toId, MAIN_ID_CHARS, false));
	}

	void Elem::resetLayout()
	{
		m_min_size = Vec<float>();

		m_scale = Vec<float>(1.0f);
		m_align = Vec<float>(0.5f);

		m_padding = Rect<float>();
		m_border = Rect<float>();
		m_margin = Rect<float>();

		m_aspect_ratio = 0.0f;

		m_axes.clear();
	}

	void Elem::applyLayout(const JsonReader &json)
	{
		json["min_size"].readDim(m_min_size);

		json["scale"].readVec(m_scale, {0.0f, 0.0f});
		json["align"].readVec(m_align);

		json["padding"].readEdges(m_padding);
		json["border"].readEdges(m_border);
		json["margin"].readEdges(m_margin);

		json["aspect_ratio"].readNum(m_aspect_ratio, 0.0f);

		json["axes"].readVector<LayoutAxis>(m_axes, [](const JsonReader &val) {
			LayoutAxis axis;
			axis.applyLayout(val);
			return axis;
		});

		m_env.requireLayout();
	}

	void Elem::resetView()
	{
		m_visible = true;
		m_noclip = false;

		m_background.resetView();
		for (size_t i = 0; i < m_slices.size(); i++) {
			m_slices[i].resetView();
		}
	}

	void Elem::applyView(const JsonReader &json)
	{
		json["visible"].readBool(m_visible);
		json["noclip"].readBool(m_noclip);

		if (auto val = json["background"]) {
			m_background.applyView(val);
		}

		json["slices"].readArray(m_slices, [](const JsonReader &val, Design &slice) {
			if (val) {
				slice.applyView(val);
			}
		});
	}

	Rect<float> Elem::getDrawEdges() const
	{
		return add_edges(m_border, m_padding);
	}

	Rect<float> Elem::getAllEdges() const
	{
		return add_edges(getDrawEdges(), m_margin);
	}

	Dim<float> Elem::getMinDrawSize() const
	{
		/* The effective minimum size for an element is based on either the
		 * content or the user minimum, depending on which is larger in each
		 * direction. The padding and borders are added to this value since they
		 * contribute to the final size of the element.
		 */
		Dim<float> total_min = dim_max(m_content_size, m_min_size) +
				edges_dim(getDrawEdges());

		/* If the element is supposed to be aspect corrected, increase its
		 * minimum size to that aspect ratio for the final minimum size.
		 * Increasing the size is performed to ensure that the element doesn't
		 * shrink below the absolute minimum above.
		 */
		if (m_aspect_ratio != 0.0f) {
			float ratio = total_min.Width / total_min.Height;
			if (ratio < m_aspect_ratio) {
				total_min.Width = total_min.Height * m_aspect_ratio;
			} else {
				total_min.Height = total_min.Width / m_aspect_ratio;
			}
		}
		return total_min;
	}

	Dim<float> Elem::getMinMarginSize() const
	{
		return getMinDrawSize() + edges_dim(m_margin);
	}

	void Elem::layout()
	{
		// Perform the leaf-to-root pass, calculating the minimum sizes of child
		// elements to determine the minimum sizes of their parent elements.
		layoutContentSize();

		/* Now that we have the minimum sizes, we perform the root-to-leaf pass,
		 * positioning elements and comparing minimum sizes to available space
		 * any allocating any spare space accordingly. We always start with the
		 * screen size as our available space since we are the root element.
		 */
		m_clip_rect = Rect<float>({0.0f, 0.0f}, m_env.getScreenSize());
		layoutDrawRect();
	}

	void Elem::layoutContentSize()
	{
		// Before anything else, zero the content size because elements start
		// out with no space and expand as they need it.
		m_content_size = Dim<float>();

		// Before we can calculate our content size, we need the content size of
		// all our children. So, instruct them to calculate it now.
		for (size_t i = 0; i < m_children.size(); i++) {
			m_env.getElem(m_children[i]).layoutContentSize();
		}

		// Now we run through the axes, calculating the minimum size for each
		// element and keeping a running total.
		for (LayoutAxis &axis : m_axes) {
			// Reset the computed fields since they will contain values from the
			// last layout.
			axis.min_size = 0.0f;
			axis.total_weight = 0.0f;

			// Count how many times each element appears in this axis since such
			// duplicate elements should add even proportions of its minimum
			// size to each column rather than the whole amount.
			std::unordered_map<size_t, size_t> elem_count;

			for (const LayoutItem &item : axis.items) {
				for (size_t index : item.elems) {
					if (index < m_children.size()) {
						// Since operator[] constructs in place, elements that
						// haven't been seen yet are zero when we access them.
						elem_count[index] += 1;
					}
				}
			}

			// Now we can compute the minimum sizes of each item in the axis.
			for (LayoutItem &item : axis.items) {
				// Again, reset the computed fields first.
				item.size = 0.0f;

				// Loop through the child element indices in this axis item to
				// find their minimum sizes.
				for (size_t index : item.elems) {
					// Ignore child indices that are out of bounds; they will
					// simply not appear in the layout.
					if (index >= m_children.size()) {
						continue;
					}

					const Elem &child = m_env.getElem(m_children[index]);
					float elem_min = dim_at(child.getMinMarginSize(), axis.dir);

					item.size = std::max(item.size, elem_min / elem_count[index]);
				}

				// Now that we have this item's minimum size, we can adjust
				// the axis's minimum size accordingly.
				axis.min_size += item.size;

				// We also compute the total weight for the axis in this stage.
				axis.total_weight += item.weight;
			}

			// Add the spacing between items to achieve the final minimum size.
			axis.min_size += axis.spacing[SPACING_BEFORE];
			axis.min_size += axis.spacing[SPACING_BETWEEN] * (axis.items.size() - 1);
			axis.min_size += axis.spacing[SPACING_AFTER];

			// Adjust the minimum size of the entire element to this new value.
			float &total_min = dim_at(m_content_size, axis.dir);
			total_min = std::max(total_min, axis.min_size);
		}
	}

	void Elem::layoutDrawRect()
	{
		/* The size of the container that the element resides in is the size of
		 * the clip rect, subtracting the element's margins since they decrease
		 * the amount of space available for the normalized coordinates of scale
		 * and align.
		 */
		Dim<float> cont_size = m_clip_rect.getSize() - edges_dim(m_margin);

		/* The base size of the element is the proportion of the clip rect given
		 * by the scale. This is then clamped to the absolute minimum size of
		 * the element. Margins do not contribute to the element size, so this
		 * size does not include them.
		 */
		Dim<float> draw_size = dim_max(Dim<float>(m_scale * cont_size), getMinDrawSize());

		if (m_aspect_ratio != 0.0f) {
			/* If our element needs to be aspect corrected, shrink the size to
			 * that aspect ratio for the final element draw size. Decreasing the
			 * size is performed to ensure that the element doesn't overflow its
			 * clip rect. This can't shrink the size below the absolute minimum
			 * since the minimum is already aspect corrected to the same ratio.
			 */
			float ratio = draw_size.Width / draw_size.Height;
			if (ratio > m_aspect_ratio) {
				draw_size.Width = draw_size.Height * m_aspect_ratio;
			} else {
				draw_size.Height = draw_size.Width / m_aspect_ratio;
			}
		}

		// The position of the element is a proportion of the remaining space in
		// the container after subtracting the size of the element from it.
		Dim<float> draw_pos(
			m_align.X * (cont_size.Width - draw_size.Width),
			m_align.Y * (cont_size.Height - draw_size.Height)
		);

		m_draw_rect = Rect<float>(draw_pos, draw_size);

		// Now that we have our content rect, we can calculate the clip rects of
		// our children.
		layoutClipRects();
	}

	void Elem::layoutClipRects()
	{
		// Calculate the rect in which all child elements will be positioned,
		// which is our own rect without border and padding.
		Rect<float> content_rect = sub_rect_edges(m_draw_rect, getDrawEdges());

		/* Elements not constrained by an axis in some direction have no size
		 * and are placed at the upper left corner of the parent element,
		 * meaning they are effectively invisible. Set this as the initial rect
		 * for all the child elements.
		 */
		Rect<float> default_clip(content_rect.UpperLeftCorner, content_rect.UpperLeftCorner);

		for (size_t i = 0; i < m_children.size(); i++) {
			Elem &child = m_env.getElem(m_children[i]);

			child.m_clip_rect = default_clip;
			child.m_clip_constrained = {false, false};
		}

		// Now we are ready to start laying out elements in each axis.
		for (LayoutAxis &axis : m_axes) {
			// Get the available space for this axis and the amount of extra
			// space we have to throw around.
			float content_space = dim_at(content_rect.getSize(), axis.dir);
			float extra_space = std::max(content_space - axis.min_size, 0.0f);

			// Depending on how we allocate extra space, we may have extra space
			// before the elements and between each element.
			float offset = 0.0f;
			float between = 0.0f;

			if (axis.total_weight > 0.0f) {
				// Some elements have a weight; spend the rest of the space on
				// each weighted element.
				for (LayoutItem &item : axis.items) {
					item.size += extra_space * (item.weight / axis.total_weight);
				}
			} else {
				// Otherwise, use the extra space as blank spacing as specified.
				switch (axis.extra_space) {
				case ExtraSpace::OUTSIDE:
					// Offset the elements by the current alignment amount.
					offset = axis.align * extra_space;
					break;
				case ExtraSpace::AROUND:
					// Place (n + 1) equal spaces around the n elements.
					between = extra_space / (axis.items.size() + 1);
					offset = between;
					break;
				case ExtraSpace::BETWEEN:
					// Place (n - 1) equal spaces between the n elements.
					between = extra_space / (axis.items.size() - 1);
					break;
				}
			}

			// Finally, we are ready to actually modify the rects of the
			// elements in this axis. Start at the starting blank offset from
			// the content rect.
			float start = content_rect.UpperLeftCorner[axis.dir] + offset;
			for (LayoutItem &item : axis.items) {
				// The ending position of the rect is the size of this axis item
				// added to the starting position.
				float end = start + item.size;

				for (size_t index : item.elems) {
					// As usual, ignore the out of bounds child indices.
					if (index >= m_children.size()) {
						continue;
					}

					// Grab the child element and its relevant rect fields in
					// this dimension.
					Elem &child = m_env.getElem(m_children[index]);
					float &before = child.m_clip_rect.UpperLeftCorner[axis.dir];
					float &after = child.m_clip_rect.LowerRightCorner[axis.dir];

					if (child.m_clip_constrained[axis.dir]) {
						// If we've constrained this element in this dimension
						// before, union the rect we gave it before with this
						// new rect to get the total rect.
						before = std::min(before, start);
						after = std::max(after, end);
					} else {
						// Otherwise, just overwrite the default rect and mark
						// it as having been constrained.
						before = start;
						after = end;

						child.m_clip_constrained[axis.dir] = true;
					}
				}

				// Having finished this axis item, add the between space to the
				// ending position to get the next starting position.
				start = end + between;
			}
		}

		// Finally, having done all the clip rect calculations, we can let the
		// child elements lay themselves out. Our work is done.
		for (size_t i = 0; i < m_children.size(); i++) {
			m_env.getElem(m_children[1]).layoutDrawRect();
		}
	}
}

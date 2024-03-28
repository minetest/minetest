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

#include "gui/elem.h"

#include "debug.h"
#include "log.h"
#include "gui/manager.h"
#include "gui/window.h"
#include "util/serialize.h"

// Include every element header for Elem::create()
#include "gui/generic_elems.h"

namespace ui
{
	std::unique_ptr<Elem> Elem::create(Type type, Window &window, std::string id)
	{
		std::unique_ptr<Elem> elem = nullptr;

#define CREATE(name, type)										\
	case name:													\
		elem = std::make_unique<type>(window, std::move(id));	\
		break

		switch (type) {
			CREATE(ELEM, Elem);
			CREATE(ROOT, Root);
		default:
			return nullptr;
		}

#undef CREATE

		// It's a pain to call reset() in the constructor of every single
		// element due to how virtual functions work in C++, so we reset
		// elements after creating them here.
		elem->reset();
		return elem;
	}

	Elem::Elem(Window &window, std::string id) :
		m_window(window),
		m_id(std::move(id)),
		m_main_box(*this)
	{}

	void Elem::reset()
	{
		m_order = (size_t)-1;

		m_parent = nullptr;
		m_children.clear();

		m_main_box.reset();
	}

	void Elem::read(std::istream &is)
	{
		u32 set_mask = readU32(is);

		if (testShift(set_mask))
			readChildren(is);
		if (testShift(set_mask))
			m_main_box.read(is);
	}

	void Elem::layout(const rf32 &parent_rect, const rf32 &parent_clip)
	{
		layoutBoxes(parent_rect, parent_clip);
		layoutChildren();
	}

	void Elem::drawAll(Canvas &canvas)
	{
		draw(canvas);

		for (Elem *child : m_children) {
			child->drawAll(canvas);
		}
	}

	void Elem::layoutBoxes(const rf32 &parent_rect, const rf32 &parent_clip)
	{
		m_main_box.layout(parent_rect, parent_clip);
	}

	void Elem::layoutChildren()
	{
		for (Elem *child : m_children) {
			child->layout(m_main_box.getChildRect(), m_main_box.getChildClip());
		}
	}

	void Elem::draw(Canvas &canvas)
	{
		m_main_box.draw(canvas);
	}

	void Elem::readChildren(std::istream &is)
	{
		u32 num_children = readU32(is);

		for (size_t i = 0; i < num_children; i++) {
			std::string id = readNullStr(is);
			Elem *child = m_window.getElem(id, true);

			if (child == nullptr) {
				continue;
			}

			/* Check if this child already has a parent before adding it as a
			 * child. Elements are deserialized in unspecified order rather
			 * than a prefix order of parents before their children, so
			 * isolated circular element refrences are still possible. However,
			 * cycles including the root are impossible, so recursion starting
			 * with the root element is safe and will always terminate.
			 */
			if (child->m_parent != nullptr) {
				errorstream << "Element \"" << id << "\" already has parent \"" <<
					child->m_parent->m_id << "\"" << std::endl;
			} else if (child == m_window.getRoot()) {
				errorstream << "Element \"" << id <<
					"\" is the root element and cannot have a parent" << std::endl;
			} else {
				m_children.push_back(child);
				child->m_parent = this;
			}
		}
	}
}

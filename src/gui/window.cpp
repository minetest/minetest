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

#include "gui/window.h"

#include "debug.h"
#include "log.h"
#include "settings.h"
#include "client/client.h"
#include "client/renderingengine.h"
#include "client/tile.h"
#include "gui/manager.h"
#include "gui/texture.h"
#include "util/serialize.h"

namespace ui
{
	WindowType toWindowType(u8 type)
	{
		if (type >= (u8)WindowType::MAX_TYPE) {
			return WindowType::HUD;
		}
		return (WindowType)type;
	}

	Elem *Window::getElem(const std::string &id, bool required)
	{
		// Empty IDs may be valid values if the element is optional.
		if (id.empty() && !required) {
			return nullptr;
		}

		// If the ID is not empty, then we need to search for an actual
		// element. Not finding one means that an error occurred.
		auto it = m_elems.find(id);
		if (it != m_elems.end()) {
			return it->second.get();
		}

		errorstream << "Element \"" << id << "\" does not exist" << std::endl;
		return nullptr;
	}

	const std::string *Window::getStyleStr(u32 index) const
	{
		if (index < m_style_strs.size()) {
			return &m_style_strs[index];
		}
		return nullptr;
	}

	void Window::reset()
	{
		m_elems.clear();
		m_ordered_elems.clear();

		m_root_elem = nullptr;

		m_style_strs.clear();
	}

	void Window::read(std::istream &is, bool opening)
	{
		std::unordered_map<Elem *, std::string> elem_contents;
		readElems(is, elem_contents);

		readRootElem(is);
		readStyles(is);

		if (opening)
			m_type = toWindowType(readU8(is));

		// Assuming no earlier step failed, we can proceed to read in all the
		// properties. Otherwise, reset the window entirely.
		if (m_root_elem != nullptr) {
			updateElems(elem_contents);
		} else {
			reset();
		}
	}

	float Window::getPixelSize() const
	{
		return g_manager.getPixelSize(m_type);
	}

	d2f32 Window::getScreenSize() const
	{
		return g_manager.getScreenSize(m_type);
	}

	void Window::drawAll()
	{
		if (m_root_elem == nullptr) {
			return;
		}

		rf32 parent_rect(getScreenSize());
		m_root_elem->layout(parent_rect, parent_rect);

		Canvas canvas(Texture::screen, getPixelSize());
		m_root_elem->drawAll(canvas);
	}

	void Window::readElems(std::istream &is,
			std::unordered_map<Elem *, std::string> &elem_contents)
	{
		// Read in all the new elements and updates to existing elements.
		u32 num_elems = readU32(is);

		std::unordered_map<std::string, std::unique_ptr<Elem>> new_elems;

		for (size_t i = 0; i < num_elems; i++) {
			u32 type = readU8(is);
			std::string id = readNullStr(is);

			// Make sure that elements have valid IDs. If the string has non-ID
			// characters in it, though, we don't particularly care.
			if (id.empty()) {
				errorstream << "Element has empty ID" << std::endl;
				continue;
			}

			// Each element has a size prefix stating how big the element is.
			// This allows new fields to be added to elements without breaking
			// compatibility. So, read it in as a string and save it for later.
			std::string contents = readStr32(is);

			// If this is a duplicate element, skip it right away.
			if (new_elems.find(id) != new_elems.end()) {
				errorstream << "Duplicate element \"" << id << "\"" << std::endl;
				continue;
			}

			/* Now we need to decide whether to create a new element or to
			 * modify the state of an already existing one. This allows
			 * changing attributes of an element (like the style or the
			 * element's children) while leaving leaving persistent state
			 * intact (such as the position of a scrollbar or the contents of a
			 * text field).
			 */
			std::unique_ptr<Elem> elem = nullptr;

			// Search for a pre-existing element.
			auto it = m_elems.find(id);

			if (it == m_elems.end() || it->second->getType() != type) {
				// If the element was not found or the existing element has the
				// wrong type, create a new element.
				elem = Elem::create((Elem::Type)type, *this, id);

				// If we couldn't create the element, the type was invalid.
				// Skip this element entirely.
				if (elem == nullptr) {
					errorstream << "Element \"" << id << "\" has an invalid type: " <<
						type << std::endl;
					continue;
				}
			} else {
				// Otherwise, use the existing element.
				elem = std::move(it->second);
			}

			// Now that we've gotten our element, reset its contents.
			elem->reset();

			// We need to read in all elements before updating each element, so
			// save the element's contents for later.
			elem_contents[elem.get()] = contents;
			new_elems.emplace(id, std::move(elem));
		}

		// Set these elements as our list of new elements.
		m_elems = std::move(new_elems);

		// Clear the ordered elements for now. They will be regenerated later.
		m_ordered_elems.clear();
	}

	void Window::readRootElem(std::istream &is)
	{
		// Get the root element of the window and make sure it's valid.
		m_root_elem = getElem(readNullStr(is), true);

		if (m_root_elem == nullptr) {
			errorstream << "Window " << m_id << " has no root element" << std::endl;
			reset();
		} else if (m_root_elem->getType() != Elem::ROOT) {
			errorstream << "Window " << m_id <<
				" has wrong type for root element" << std::endl;
			reset();
		}
	}

	void Window::readStyles(std::istream &is)
	{
		// Styles are stored in their raw binary form; every time a style needs
		// to be recalculated, these binary strings can be applied one over the
		// other, resulting in automatic cascading styles.
		u32 num_styles = readU32(is);
		m_style_strs.clear();

		for (size_t i = 0; i < num_styles; i++) {
			m_style_strs.push_back(readStr16(is));
		}
	}

	void Window::updateElems(std::unordered_map<Elem *, std::string> &elem_contents)
	{
		// Now that we have a fully updated window, we can update each element
		// with its contents. We couldn't do this before because elements need
		// to be able to call getElem() and getStyleStr().
		for (auto &contents : elem_contents) {
			auto is = newIs(std::move(contents.second));
			contents.first->read(is);
		}

		// Check the depth of the element tree; if it's too deep, there's
		// potential for stack overflow.
		if (!checkTree(m_root_elem, 1)) {
			reset();
			return;
		}

		// Update the ordering of the elements so we can do iteration rather
		// than recursion when searching through the elements in order.
		updateElemOrdering(m_root_elem, 0);
	}

	bool Window::checkTree(Elem *elem, size_t depth) const
	{
		if (depth > MAX_TREE_DEPTH) {
			errorstream << "Window " << m_id <<
					" exceeds max tree depth: " << MAX_TREE_DEPTH << std::endl;
			return false;
		}

		for (Elem *child : elem->getChildren()) {
			if (child->getType() == Elem::ROOT) {
				errorstream << "Element of root type \"" << child->getId() <<
					"\" is not root of window" << std::endl;
				return false;
			}

			if (!checkTree(child, depth + 1)) {
				return false;
			}
		}

		return true;
	}

	size_t Window::updateElemOrdering(Elem *elem, size_t order)
	{
		// The parent gets ordered before its children since the ordering of
		// elements follows draw order.
		elem->setOrder(order);
		m_ordered_elems.push_back(elem);

		for (Elem *child : elem->getChildren()) {
			// Order this element's children using the next index after the
			// parent, returning the index of the last child element.
			order = updateElemOrdering(child, order + 1);
		}

		return order;
	}
}

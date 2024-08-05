/*
Minetest
Copyright (C) 2024 grorp, Gregor Parzefall <grorp@posteo.de>

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
#include <memory>
#include "IGUIElement.h"

// We cannot use irr_ptr for Irrlicht GUI elements we own.
// Option 1: Pass IGUIElement* returned by IGUIEnvironment::add* into irr_ptr
//           constructor.
//           -> We steal the reference owned by IGUIEnvironment and drop it later,
//           causing the IGUIElement to be deleted while IGUIEnvironment still
//           references it.
// Option 2: Pass IGUIElement* returned by IGUIEnvironment::add* into irr_ptr::grab.
//           -> We add another reference and drop it later, but since IGUIEnvironment
//           still references the IGUIElement, it is never deleted.
// To make IGUIEnvironment drop its reference to the IGUIElement, we have to call
// IGUIElement::remove, so that's what we'll do.
template <typename T>
std::shared_ptr<T> grab_gui_element(T *element)
{
	static_assert(std::is_base_of_v<irr::gui::IGUIElement, T>,
			"grab_gui_element only works for IGUIElement");
	return std::shared_ptr<T>(element, [](T *e) {
		e->remove();
	});
}

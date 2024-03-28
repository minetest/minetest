/*
Minetest
Copyright (C) 2024 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include "gui/generic_elems.h"

#include "debug.h"
#include "log.h"
#include "gui/manager.h"
#include "util/serialize.h"

namespace ui
{
	void Root::reset()
	{
		Elem::reset();

		m_backdrop_box.reset();
	}

	void Root::read(std::istream &is)
	{
		auto super = newIs(readStr32(is));
		Elem::read(super);

		u32 set_mask = readU32(is);

		if (testShift(set_mask))
			m_backdrop_box.read(is);
	}

	void Root::layoutBoxes(const rf32 &parent_rect, const rf32 &parent_clip)
	{
		m_backdrop_box.layout(parent_rect, parent_clip);
		Elem::layoutBoxes(m_backdrop_box.getChildRect(), m_backdrop_box.getChildClip());
	}

	void Root::draw(Canvas &canvas)
	{
		m_backdrop_box.draw(canvas);
		Elem::draw(canvas);
	}
}

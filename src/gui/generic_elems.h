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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include "gui/box.h"
#include "gui/elem.h"

#include <iostream>
#include <string>

namespace ui
{
	class Root : public Elem
	{
	private:
		Box m_backdrop_box;

	public:
		Root(Window &window, std::string id) :
			Elem(window, std::move(id)),
			m_backdrop_box(*this)
		{}

		virtual Type getType() const override { return ROOT; }

		virtual void reset() override;
		virtual void read(std::istream &is) override;

	protected:
		virtual void layoutBoxes(const rf32 &parent_rect, const rf32 &parent_clip);

		virtual void draw(Canvas &canvas);
	};
}

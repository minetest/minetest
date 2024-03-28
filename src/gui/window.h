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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include "gui/elem.h"
#include "util/basic_macros.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui
{
	// Serialized enum; do not change order of entries.
	enum class WindowType
	{
		BG,
		MASK,
		HUD,
		MESSAGE,
		GUI,
		FG,

		MAX_TYPE,
	};

	WindowType toWindowType(u8 type);

	class Window
	{
	private:
		static constexpr size_t MAX_TREE_DEPTH = 64;

		u64 m_id;
		WindowType m_type = WindowType::GUI;

		std::unordered_map<std::string, std::unique_ptr<Elem>> m_elems;
		std::vector<Elem *> m_ordered_elems;

		Elem *m_root_elem;

		std::vector<std::string> m_style_strs;

	public:
		Window(u64 id) :
			m_id(id)
		{
			reset();
		}

		DISABLE_CLASS_COPY(Window)
		ALLOW_CLASS_MOVE(Window)

		u64 getId() const { return m_id; }

		WindowType getType() const { return m_type; }

		const std::vector<Elem *> &getElems() { return m_ordered_elems; }
		Elem *getElem(const std::string &id, bool required);

		Elem *getRoot() { return m_root_elem; }

		const std::string *getStyleStr(u32 index) const;

		void reset();
		void read(std::istream &is, bool opening);

		float getPixelSize() const;
		d2f32 getScreenSize() const;

		void drawAll();

	private:
		void readElems(std::istream &is,
				std::unordered_map<Elem *, std::string> &elem_contents);
		void readRootElem(std::istream &is);
		void readStyles(std::istream &is);

		void updateElems(std::unordered_map<Elem *, std::string> &elem_contents);
		bool checkTree(Elem *elem, size_t depth) const;
		size_t updateElemOrdering(Elem *elem, size_t order);
	};
}

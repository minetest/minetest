/*
Minetest
Copyright (C) 2020 Hugues Ross <hugues.ross@gmail.com>

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

#include <string>

#include "irrlichttypes_extrabloated.h"

enum FormspecFieldType
{
	f_Button,
	f_Table,
	f_TabHeader,
	f_CheckBox,
	f_DropDown,
	f_ScrollBar,
	f_Box,
	f_ItemImage,
	f_HyperText,
	f_AnimatedImage,
	f_Unknown,
};

struct FieldSpec
{
	FieldSpec() = default;

	FieldSpec(const std::string &name, const std::wstring &label,
			const std::wstring &default_text, s32 id, int priority = 0,
			irr::gui::ECURSOR_ICON cursor_icon = irr::gui::ECI_NORMAL) :
			fname(name),
			flabel(label),
			fdefault(unescape_enriched(translate_string(default_text))),
			fid(id), send(false), ftype(f_Unknown), is_exit(false),
			priority(priority), fcursor_icon(cursor_icon)
	{
	}

	std::string fname;
	std::wstring flabel;
	std::wstring fdefault;
	s32 fid;
	bool send;
	FormspecFieldType ftype;
	bool is_exit;
	// Draw priority for formspec version < 3
	int priority;
	irr::core::rect<s32> rect;
	irr::gui::ECURSOR_ICON fcursor_icon;
};

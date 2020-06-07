#pragma once

#include "irrlichttypes_extrabloated.h"

typedef enum {
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
	f_Unknown
} FormspecFieldType;

struct FieldSpec
{
	FieldSpec() = default;

	FieldSpec(const std::string &name, const std::wstring &label,
			  const std::wstring &default_text, s32 id, int priority = 0,
			  gui::ECURSOR_ICON cursor_icon = irr::gui::ECI_NORMAL) :
			fname(name),
			flabel(label),
			fdefault(unescape_enriched(translate_string(default_text))),
			fid(id),
			send(false),
			ftype(f_Unknown),
			is_exit(false),
			priority(priority),
			fcursor_icon(cursor_icon)
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
	core::rect<s32> rect;
	gui::ECURSOR_ICON fcursor_icon;
};

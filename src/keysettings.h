/*
 Minetest
 Copyright (C) 2010-2013, 2016 celeron55, Perttu Ahola <celeron55@gmail.com>
 Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
 Copyright (C) 2013 teddydestodes <derkomtur@schattengang.net>

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

#ifndef KEYSETTINGS_HEADER
#define KEYSETTINGS_HEADER

#include "keycode.h"
#include <string>
#include <vector>

struct KeySetting {
	int index;
	int id;
	const wchar_t *button_name;
	KeyPress key;
	std::string setting_name;
};

struct KeySettings {
	std::vector<KeySetting> settings;
	std::vector<KeyCommand> aliases;

	KeySettings();
	~KeySettings();

	void addKey(int id, const wchar_t *button_name, const std::string &setting_name);
	void addAlias(const KeyCommand &key);
	void refreshKeys();
	void refreshAliases();
	std::wstring keyUsedBy(int id, const KeyPress &key, bool modifier_shift,
			bool modifier_control, s32 current_command_id=-1);
	void setKeySettings();
	void setAliasSettings();
};

enum
{
	GUI_ID_OFFSET_KEY_FORWARD_BUTTON,
	GUI_ID_OFFSET_KEY_BACKWARD_BUTTON,
	GUI_ID_OFFSET_KEY_LEFT_BUTTON,
	GUI_ID_OFFSET_KEY_RIGHT_BUTTON,
	GUI_ID_OFFSET_KEY_USE_BUTTON,
	GUI_ID_OFFSET_KEY_FLY_BUTTON,
	GUI_ID_OFFSET_KEY_FAST_BUTTON,
	GUI_ID_OFFSET_KEY_JUMP_BUTTON,
	GUI_ID_OFFSET_KEY_NOCLIP_BUTTON,
	GUI_ID_OFFSET_KEY_CINEMATIC_BUTTON,
	GUI_ID_OFFSET_KEY_CHAT_BUTTON,
	GUI_ID_OFFSET_KEY_CMD_BUTTON,
	GUI_ID_OFFSET_KEY_CONSOLE_BUTTON,
	GUI_ID_OFFSET_KEY_SNEAK_BUTTON,
	GUI_ID_OFFSET_KEY_DROP_BUTTON,
	GUI_ID_OFFSET_KEY_INVENTORY_BUTTON,
	GUI_ID_OFFSET_KEY_DUMP_BUTTON,
	GUI_ID_OFFSET_KEY_RANGE_BUTTON,
};

#endif

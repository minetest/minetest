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

#include "keysettings.h"
#include "irrlichttypes_extrabloated.h"
#include "gettext.h"
#include "util/string.h"

KeySettings::KeySettings()
{
	refreshKeys();
	refreshAliases();
}

KeySettings::~KeySettings()
{
	for (std::vector<KeySetting>::iterator iter = settings.begin();
			iter != settings.end(); ++iter) {
		delete[] iter->button_name;
	}
}

void KeySettings::addKey(int id, const wchar_t *button_name, const std::string &setting_name)
{
	settings.push_back(KeySetting());
	KeySetting *k = &settings[settings.size()-1];
	k->index = settings.size()-1;
	k->id = id;

	k->button_name = button_name;
	k->setting_name = setting_name;
	k->key = getKeySetting(k->setting_name.c_str());
}

void KeySettings::addAlias(const KeyCommand &key)
{
	aliases.push_back(key);
}

void KeySettings::refreshKeys()
{
	for (std::vector<KeySetting>::iterator iter = settings.begin();
			iter != settings.end(); ++iter) {
		delete[] iter->button_name;
	}

	addKey(GUI_ID_OFFSET_KEY_FORWARD_BUTTON,   wgettext("Forward"),          "keymap_forward");
	addKey(GUI_ID_OFFSET_KEY_BACKWARD_BUTTON,  wgettext("Backward"),         "keymap_backward");
	addKey(GUI_ID_OFFSET_KEY_LEFT_BUTTON,      wgettext("Left"),             "keymap_left");
	addKey(GUI_ID_OFFSET_KEY_RIGHT_BUTTON,     wgettext("Right"),            "keymap_right");
	addKey(GUI_ID_OFFSET_KEY_USE_BUTTON,       wgettext("Use"),              "keymap_special1");
	addKey(GUI_ID_OFFSET_KEY_JUMP_BUTTON,      wgettext("Jump"),             "keymap_jump");
	addKey(GUI_ID_OFFSET_KEY_SNEAK_BUTTON,     wgettext("Sneak"),            "keymap_sneak");
	addKey(GUI_ID_OFFSET_KEY_DROP_BUTTON,      wgettext("Drop"),             "keymap_drop");
	addKey(GUI_ID_OFFSET_KEY_INVENTORY_BUTTON, wgettext("Inventory"),        "keymap_inventory");
	addKey(GUI_ID_OFFSET_KEY_CHAT_BUTTON,      wgettext("Chat"),             "keymap_chat");
	addKey(GUI_ID_OFFSET_KEY_CMD_BUTTON,       wgettext("Command"),          "keymap_cmd");
	addKey(GUI_ID_OFFSET_KEY_CONSOLE_BUTTON,   wgettext("Console"),          "keymap_console");
	addKey(GUI_ID_OFFSET_KEY_FLY_BUTTON,       wgettext("Toggle fly"),       "keymap_freemove");
	addKey(GUI_ID_OFFSET_KEY_FAST_BUTTON,      wgettext("Toggle fast"),      "keymap_fastmove");
	addKey(GUI_ID_OFFSET_KEY_CINEMATIC_BUTTON, wgettext("Toggle Cinematic"), "keymap_cinematic");
	addKey(GUI_ID_OFFSET_KEY_NOCLIP_BUTTON,    wgettext("Toggle noclip"),    "keymap_noclip");
	addKey(GUI_ID_OFFSET_KEY_RANGE_BUTTON,     wgettext("Range select"),     "keymap_rangeselect");
	addKey(GUI_ID_OFFSET_KEY_DUMP_BUTTON,      wgettext("Print stacks"),     "keymap_print_debug_stacks");
}

void KeySettings::refreshAliases()
{
	aliases.clear();

	for (size_t i = 0; i<getCommandKeySettingCount(); ++i)
		addAlias(getCommandKeySetting(i));
}

std::wstring KeySettings::keyUsedBy(int id, const KeyPress &key, bool modifier_shift,
		bool modifier_control, s32 current_command_id)
{
	for (std::vector<KeySetting>::iterator it = settings.begin(); it != settings.end(); ++it)
	{
		if (it->id == id)
			continue;
		if (it->key == key)
			return std::wstring(it->button_name);
	}
	for (std::vector<KeyCommand>::iterator it = aliases.begin(); it != aliases.end(); ++it)
	{
		if (current_command_id == it - aliases.begin())
			continue;
		if (it->key == key)
		{
			if (it->modifier_shift == modifier_shift &&
					it->modifier_control == modifier_control)
				return std::wstring(L"chat command \"" + narrow_to_wide(it->setting_name) + L"\"");
		}
	}
	return L"";
}


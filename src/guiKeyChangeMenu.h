/*
 Minetest-c55
 Copyright (C) 2010-11 celeron55, Perttu Ahola <celeron55@gmail.com>
 Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>
 Copyright (C) 2011 teddydestodes <derkomtur@schattengang.net>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef GUIKEYCHANGEMENU_HEADER
#define GUIKEYCHANGEMENU_HEADER

#include "common_irrlicht.h"
#include "utility.h"
#include "modalMenu.h"
#include "client.h"
#include "gettext.h"
#include <string>

static const char *KeyNames[] =
	{ "-", gettext("Left Button"), gettext("Right Button"), gettext("Cancel"), gettext("Middle Button"), gettext("X Button 1"),
			gettext("X Button 2"), "-", gettext("Back"), gettext("Tab"), "-", "-", gettext("Clear"), gettext("Return"), "-",
			"-", gettext("Shift"), gettext("Control"), gettext("Menu"), gettext("Pause"), gettext("Capital"), gettext("Kana"), "-",
			gettext("Junja"), gettext("Final"), gettext("Kanji"), "-", gettext("Escape"), gettext("Convert"), gettext("Nonconvert"),
			gettext("Accept"), gettext("Mode Change"), gettext("Space"), gettext("Priot"), gettext("Next"), gettext("End"), gettext("Home"),
			gettext("Left"), gettext("Up"), gettext("Right"), gettext("Down"), gettext("Select"), gettext("Print"), gettext("Execute"),
			gettext("Snapshot"), gettext("Insert"), gettext("Delete"), gettext("Help"), "0", "1", "2", "3", "4", "5",
			"6", "7", "8", "9", "-", "-", "-", "-", "-", "-", "-", "A", "B", "C",
			"D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q",
			"R", "S", "T", "U", "V", "W", "X", "Y", "Z", gettext("Left Windows"),
			gettext("Right Windows"), gettext("Apps"), "-", gettext("Sleep"), gettext("Numpad 0"), gettext("Numpad 1"),
			gettext("Numpad 2"), gettext("Numpad 3"), gettext("Numpad 4"), gettext("Numpad 5"), gettext("Numpad 6"), gettext("Numpad 7"),
			gettext("Numpad 8"), gettext("Numpad 9"), gettext("Numpad *"), gettext("Numpad +"), gettext("Numpad /"), gettext("Numpad -"),
			"Numpad .", "Numpad /", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
			"F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18",
			"F19", "F20", "F21", "F22", "F23", "F24", "-", "-", "-", "-", "-", "-",
			"-", "-", gettext("Num Lock"), gettext("Scroll Lock"), "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", gettext("Left Shift"), gettext("Right Shight"),
			gettext("Left Control"), gettext("Right Control"), gettext("Left Menu"), gettext("Right Menu"), "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", gettext("Plus"), gettext("Comma"), gettext("Minus"), gettext("Period"), "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", gettext("Attn"), gettext("CrSel"),
			gettext("ExSel"), gettext("Erase OEF"), gettext("Play"), gettext("Zoom"), gettext("PA1"), gettext("OEM Clear"), "-" };
	enum
	{
		GUI_ID_BACK_BUTTON = 101, GUI_ID_ABORT_BUTTON, GUI_ID_SCROLL_BAR,
		//buttons
		GUI_ID_KEY_FORWARD_BUTTON,
		GUI_ID_KEY_BACKWARD_BUTTON,
		GUI_ID_KEY_LEFT_BUTTON,
		GUI_ID_KEY_RIGHT_BUTTON,
		GUI_ID_KEY_USE_BUTTON,
		GUI_ID_KEY_FLY_BUTTON,
		GUI_ID_KEY_FAST_BUTTON,
		GUI_ID_KEY_JUMP_BUTTON,
		GUI_ID_KEY_CHAT_BUTTON,
		GUI_ID_KEY_SNEAK_BUTTON,
		GUI_ID_KEY_INVENTORY_BUTTON,
		GUI_ID_KEY_DUMP_BUTTON,
		GUI_ID_KEY_RANGE_BUTTON
	};

class GUIKeyChangeMenu: public GUIModalMenu
{
public:
	GUIKeyChangeMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent,
			s32 id, IMenuManager *menumgr);
	~GUIKeyChangeMenu();

	void removeChildren();
	/*
	 Remove and re-add (or reposition) stuff
	 */
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	bool acceptInput();

	bool OnEvent(const SEvent& event);

private:

	void init_keys();

	bool resetMenu();

	gui::IGUIButton *forward;
	gui::IGUIButton *backward;
	gui::IGUIButton *left;
	gui::IGUIButton *right;
	gui::IGUIButton *use;
	gui::IGUIButton *sneak;
	gui::IGUIButton *jump;
	gui::IGUIButton *inventory;
	gui::IGUIButton *fly;
	gui::IGUIButton *fast;
	gui::IGUIButton *range;
	gui::IGUIButton *dump;
	gui::IGUIButton *chat;

	u32 activeKey;
	u32 key_forward;
	u32 key_backward;
	u32 key_left;
	u32 key_right;
	u32 key_use;
	u32 key_sneak;
	u32 key_jump;
	u32 key_inventory;
	u32 key_fly;
	u32 key_fast;
	u32 key_range;
	u32 key_chat;
	u32 key_dump;
};

#endif


/*
 Minetest-delta
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
#include <string>

static const char *KeyNames[] =
	{ "-", "Left Button", "Right Button", "Cancel", "Middle Button", "X Button 1",
			"X Button 2", "-", "Back", "Tab", "-", "-", "Clear", "Return", "-",
			"-", "Shift", "Control", "Menu", "Pause", "Capital", "Kana", "-",
			"Junja", "Final", "Kanji", "-", "Escape", "Convert", "Nonconvert",
			"Accept", "Mode Change", "Space", "Priot", "Next", "End", "Home",
			"Left", "Up", "Right", "Down", "Select", "Print", "Execute",
			"Snapshot", "Insert", "Delete", "Help", "0", "1", "2", "3", "4", "5",
			"6", "7", "8", "9", "-", "-", "-", "-", "-", "-", "-", "A", "B", "C",
			"D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q",
			"R", "S", "T", "U", "V", "W", "X", "Y", "Z", "Left Windows",
			"Right Windows", "Apps", "-", "Sleep", "Numpad 0", "Numpad 1",
			"Numpad 2", "Numpad 3", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad 7",
			"Numpad 8", "Numpad 9", "Numpad *", "Numpad +", "Numpad /", "Numpad -",
			"Numpad .", "Numpad /", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
			"F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", "F17", "F18",
			"F19", "F20", "F21", "F22", "F23", "F24", "-", "-", "-", "-", "-", "-",
			"-", "-", "Num Lock", "Scroll Lock", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "Left Shift", "Right Shight",
			"Left Control", "Right Control", "Left Menu", "Right Menu", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "Plus", "Comma", "Minus", "Period", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-",
			"-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "-", "Attn", "CrSel",
			"ExSel", "Erase OEF", "Play", "Zoom", "PA1", "OEM Clear", "-" };
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


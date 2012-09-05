/*
 Minetest-c55
 Copyright (C) 2010-11 celeron55, Perttu Ahola <celeron55@gmail.com>
 Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>
 Copyright (C) 2011 teddydestodes <derkomtur@schattengang.net>

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

#ifndef GUIKEYCHANGEMENU_HEADER
#define GUIKEYCHANGEMENU_HEADER

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include "client.h"
#include "gettext.h"
#include "keycode.h"
#include <string>
#include <vector>

typedef struct {
	int id;
	wchar_t *button_name;
	KeyPress key;
	std::string setting_name;
	gui::IGUIButton *button;
} key_setting;


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

	void add_key(int id, std::string setting_name, std::string button_name);

	bool shift_down;
	
	s32 activeKey;
	
	std::vector<KeyPress> key_used;
	gui::IGUIStaticText *key_used_text;
	std::vector<key_setting *> key_settings;
};

#endif


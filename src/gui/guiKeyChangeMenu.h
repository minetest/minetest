/*
 Minetest
 Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include "gettext.h"
#include "client/keycode.h"
#include <string>
#include <vector>

class ISimpleTextureSource;

struct key_setting
{
	int id;
	const wchar_t *button_name;
	KeyPress key;
	std::string setting_name;
	gui::IGUIButton *button;
};

class GUIKeyChangeMenu : public GUIModalMenu
{
public:
	GUIKeyChangeMenu(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
			IMenuManager *menumgr, ISimpleTextureSource *tsrc);
	~GUIKeyChangeMenu();

	/*
	 Remove and re-add (or reposition) stuff
	 */
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	bool acceptInput();

	bool OnEvent(const SEvent &event);

	bool pausesGame() { return true; }

protected:
	std::wstring getLabelByID(s32 id) { return L""; }
	std::string getNameByID(s32 id) { return ""; }

private:
	void init_keys();

	bool resetMenu();

	void add_key(int id, const wchar_t *button_name, const std::string &setting_name);

	bool shift_down = false;

	key_setting *active_key = nullptr;
	gui::IGUIStaticText *key_used_text = nullptr;
	std::vector<key_setting *> key_settings;
	ISimpleTextureSource *m_tsrc;
};

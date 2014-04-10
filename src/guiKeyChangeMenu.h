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

#ifndef GUIKEYCHANGEMENU_HEADER
#define GUIKEYCHANGEMENU_HEADER

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include "client.h"
#include "gettext.h"
#include "keycode.h"
#include <string>
#include <vector>
#include <set>

typedef struct {
	int id;
	wchar_t *button_name;
	KeyPress key;
	std::string setting_name;
	gui::IGUIButton *button;
} key_setting;

typedef struct {
	int id;
	KeyCommand key;
	std::string setting_name;
	std::string command_alias;
} key_alias_setting;


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
    std::wstring keyUsedBy(int id, const KeyPress &key, bool modifier_shift, bool modifier_control);
	void add_key(int id, wchar_t* button_name, std::string setting_name);
    void add_command_alias_key(const KeyCommand &key);
    void commandComboChanged();

	bool shift_down;
	bool control_down;

	s32 activeKey;

	gui::IGUIStaticText *key_used_text;
	std::vector<key_setting *> key_settings;
	std::vector<KeyCommand> key_alias_settings;
	IMenuManager *m_menumgr_copy;
	gui::IGUIElement* m_parent;

    s32 m_command_active_id;
    gui::IGUIEditBox *m_command_name;
    gui::IGUIStaticText *m_command_label;
    gui::IGUIEditBox *m_command;
    gui::IGUIStaticText *m_command_key_label;
    gui::IGUIButton *m_command_key;
    gui::IGUIButton *m_command_add;
    gui::IGUIButton *m_command_remove;
    gui::IGUIComboBox *m_command_combo;
    bool m_command_adding;
};

#endif


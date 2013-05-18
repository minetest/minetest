/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef GUICONFIGUREWORLD_HEADER
#define GUICONFIGUREWORLD_HEADER

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include "mods.h"
#include "subgame.h"
#include "settings.h"


namespace irr{
	namespace gui{
		class IGUITreeViewNode;
	}
}

class GUIConfigureWorld : public GUIModalMenu
{
public:
	GUIConfigureWorld(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
			  IMenuManager *menumgr, WorldSpec wspec);

	void regenerateGui(v2u32 screensize);

	void drawMenu();

	bool OnEvent(const SEvent& event);

private:
	WorldSpec m_wspec;
	SubgameSpec m_gspec;

	// tree of installed add-on mods. key is the mod name, modpacks
	// are not expanded.
	std::map<std::string, ModSpec> m_addontree;

	// like m_addontree, but modpacks are expanded.
	std::map<std::string, ModSpec> m_addonmods;

	// list of game mods (flattened)
	std::map<std::string, ModSpec> m_gamemods;

	// list of world mods (flattened)
	std::map<std::string, ModSpec> m_worldmods;
	
	// for each mod, the set of mods depending on it
	std::multimap<std::string, std::string> m_reverse_depends;

	// the settings in the world.mt file
	Settings m_settings;

	// maps modnames to nodes in m_treeview
	std::map<std::string,gui::IGUITreeViewNode*> m_nodes;

	gui::IGUIStaticText* m_modname_text;
	gui::IGUITreeView* m_treeview;
	gui::IGUIButton* m_enableall;
	gui::IGUIButton* m_disableall;
	gui::IGUICheckBox* m_enabled_checkbox;
	gui::IGUIListBox* m_dependencies_listbox;
	gui::IGUIListBox* m_rdependencies_listbox;
	void buildTreeView(std::map<std::string,ModSpec> mods, 
					   gui::IGUITreeViewNode* node);
	void adjustSidebar();
	void enableAllMods(std::map<std::string,ModSpec> mods, bool enable);
	void setEnabled(std::string modname, bool enable)
	{
		if(enable)
			enableMod(modname);
		else
			disableMod(modname);
	};

	void enableMod(std::string modname);
	void disableMod(std::string modname);

	// hack to work around wonky handling of double-click in
	// irrlicht. store selected index of listbox items here so event
	// handling can check whether it was a real double click on the
	// same item. (irrlicht also reports a double click if you rapidly
	// select two different items.)
	int selecting_dep;
	int selecting_rdep;

	IMenuManager* m_menumgr;
};
#endif

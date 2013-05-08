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


#include <iostream> 
#include <string>
#include <map>

#include "guiConfigureWorld.h"
#include "guiMessageMenu.h"
#include <IGUIButton.h>
#include <IGUICheckBox.h>
#include <IGUIListBox.h>
#include <IGUIStaticText.h>
#include <IGUITreeView.h>
#include "gettext.h"
#include "util/string.h"
#include "settings.h"
#include "filesys.h"

enum
{
	GUI_ID_MOD_TREEVIEW = 101,
	GUI_ID_ENABLED_CHECKBOX,
	GUI_ID_ENABLEALL,
	GUI_ID_DISABLEALL,
	GUI_ID_DEPENDS_LISTBOX,
	GUI_ID_RDEPENDS_LISTBOX,
	GUI_ID_CANCEL,
	GUI_ID_SAVE
};

#define QUESTIONMARK_STR L"?"
#define CHECKMARK_STR    L"\411"
#define CROSS_STR        L"\403"

GUIConfigureWorld::GUIConfigureWorld(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr, WorldSpec wspec):
	GUIModalMenu(env, parent, id, menumgr),
	m_wspec(wspec),
	m_gspec(findWorldSubgame(m_wspec.path)),
	m_menumgr(menumgr)
{
	//will be initialized in regenerateGUI()
	m_treeview=NULL;

	// game mods
	m_gamemods = flattenModTree(getModsInPath(m_gspec.gamemods_path));

	// world mods
	std::string worldmods_path = wspec.path + DIR_DELIM + "worldmods";
	m_worldmods = flattenModTree(getModsInPath(worldmods_path));

	// fill m_addontree with add-on mods
	std::set<std::string> paths = m_gspec.addon_mods_paths;
	for(std::set<std::string>::iterator it=paths.begin();
		it != paths.end(); ++it)
	{
		std::map<std::string,ModSpec> mods = getModsInPath(*it);
		m_addontree.insert(mods.begin(), mods.end());
	}

	// expand modpacks
	m_addonmods = flattenModTree(m_addontree);

	// collect reverse dependencies
	for(std::map<std::string, ModSpec>::iterator it = m_addonmods.begin();
		it != m_addonmods.end(); ++it)
	{
		std::string modname = (*it).first;
		ModSpec mod = (*it).second;
		for(std::set<std::string>::iterator dep_it = mod.depends.begin();
			dep_it != mod.depends.end(); ++dep_it)
		{
			m_reverse_depends.insert(std::make_pair((*dep_it),modname));
		}
	}

	m_settings.readConfigFile((m_wspec.path + DIR_DELIM + "world.mt").c_str());
	std::vector<std::string> names = m_settings.getNames();

	// mod_names contains the names of mods mentioned in the world.mt file
	std::set<std::string> mod_names;
	for(std::vector<std::string>::iterator it = names.begin(); 
		it != names.end(); ++it)
	{	
		std::string name = *it;  
		if (name.compare(0,9,"load_mod_")==0)
			mod_names.insert(name.substr(9));
	}

	// find new mods (installed but not mentioned in world.mt)
	for(std::map<std::string, ModSpec>::iterator it = m_addonmods.begin();
		it != m_addonmods.end(); ++it)
	{
		std::string modname = (*it).first;
		ModSpec mod = (*it).second;
		// a mod is new if it is not a modpack, and does not occur in
		// mod_names
		if(!mod.is_modpack &&
		   mod_names.count(modname) == 0)
			m_new_mod_names.insert(modname);
	}
	if(!m_new_mod_names.empty())
	{
		wchar_t* text = wgettext("Warning: Some mods are not configured yet.\n"
				"They will be enabled by default when you save the configuration.  ");
		GUIMessageMenu *menu = 
			new GUIMessageMenu(Environment, Parent, -1, m_menumgr, text);
		menu->drop();
		delete[] text;
	}
	

	// find missing mods (mentioned in world.mt, but not installed)
	std::set<std::string> missing_mods;
	for(std::set<std::string>::iterator it = mod_names.begin();
		it != mod_names.end(); ++it)
	{
		std::string modname = *it;
		if(m_addonmods.count(modname) == 0)
			missing_mods.insert(modname);
	}
	if(!missing_mods.empty())
	{
		wchar_t* text = wgettext("Warning: Some configured mods are missing.\n"
				"Their setting will be removed when you save the configuration.  ");
		GUIMessageMenu *menu = 
			new GUIMessageMenu(Environment, Parent, -1, m_menumgr, text);
		delete[] text;
		for(std::set<std::string>::iterator it = missing_mods.begin();
			it != missing_mods.end(); ++it)
			m_settings.remove("load_mod_"+(*it));
		menu->drop();
	}	
}

void GUIConfigureWorld::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}


void GUIConfigureWorld::regenerateGui(v2u32 screensize)
{

	/*
		Remove stuff
	*/
	removeChildren();
	
	/*
		Calculate new sizes and positions
	*/
	core::rect<s32> rect(
			screensize.X/2 - 580/2,
			screensize.Y/2 - 300/2,
			screensize.X/2 + 580/2,
			screensize.Y/2 + 300/2
	);
	
	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 size = rect.getSize();

	v2s32 topleft = v2s32(10, 10);

	/*
		Add stuff
	*/
	changeCtype("");
	{
		core::rect<s32> rect(0, 0, 200, 20);
		rect += topleft;
		//proper text is set below, when a mod is selected
		m_modname_text = Environment->addStaticText(L"Mod: N/A", rect, false, 
													false, this, -1);
	}
	{
		core::rect<s32> rect(0, 0, 200, 20);
		rect += v2s32(0, 25) + topleft;
		wchar_t* text = wgettext("enabled");
		m_enabled_checkbox = 
			Environment->addCheckBox(false, rect, this, GUI_ID_ENABLED_CHECKBOX, 
									 text);
		delete[] text;
		m_enabled_checkbox->setVisible(false);
	}
	{
		core::rect<s32> rect(0, 0, 85, 30);
		rect = rect + v2s32(0, 25) + topleft;
		wchar_t* text = wgettext("Enable All");
		m_enableall = Environment->addButton(rect, this, GUI_ID_ENABLEALL,
											 text);
		delete[] text;
		m_enableall->setVisible(false);
	}
	{
		core::rect<s32> rect(0, 0, 85, 30);
		rect = rect + v2s32(115, 25) + topleft;
		wchar_t* text = wgettext("Disable All");
		m_disableall = Environment->addButton(rect, this, GUI_ID_DISABLEALL, text );
		delete[] text;
		m_disableall->setVisible(false);
	}
	{
		core::rect<s32> rect(0, 0, 200, 20);
		rect += v2s32(0, 60) + topleft;
		wchar_t* text = wgettext("depends on:");
		Environment->addStaticText(text, rect, false, false, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 200, 85);
		rect += v2s32(0, 80) + topleft;
		m_dependencies_listbox = 
			Environment->addListBox(rect, this, GUI_ID_DEPENDS_LISTBOX, true);
	}
	{
		core::rect<s32> rect(0, 0, 200, 20);
		rect += v2s32(0, 175) + topleft;
		wchar_t* text = wgettext("is required by:");
		Environment->addStaticText( text, rect, false, false, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 200, 85);
		rect += v2s32(0, 195) + topleft;
		m_rdependencies_listbox = 
			Environment->addListBox(rect,this, GUI_ID_RDEPENDS_LISTBOX,true);
	}
	{
		core::rect<s32> rect(0, 0, 340, 250);
		rect += v2s32(220, 0) + topleft;
		m_treeview = Environment->addTreeView(rect, this,
											  GUI_ID_MOD_TREEVIEW,true);
		gui::IGUITreeViewNode* node 
			= m_treeview->getRoot()->addChildBack(L"Add-Ons");
		buildTreeView(m_addontree, node);
	}
	{
		core::rect<s32> rect(0, 0, 120, 30);
		rect = rect + v2s32(330, 270) - topleft;
		wchar_t* text = wgettext("Cancel");
		Environment->addButton(rect, this, GUI_ID_CANCEL, text);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 120, 30);
		rect = rect + v2s32(460, 270) - topleft;
		wchar_t* text = wgettext("Save");
		Environment->addButton(rect, this, GUI_ID_SAVE, text);
		delete[] text;
	}
	changeCtype("C");

	// at start, none of the treeview nodes is selected, so we select
	// the first element in the treeview of mods manually here.
	if(m_treeview->getRoot()->hasChilds())
	{
		m_treeview->getRoot()->getFirstChild()->setExpanded(true);
		m_treeview->getRoot()->getFirstChild()->setSelected(true);
		// Because a manual ->setSelected() doesn't cause an event, we
		// have to do this here:
		adjustSidebar();
	}
}

bool GUIConfigureWorld::OnEvent(const SEvent& event)
{

	gui::IGUITreeViewNode* selected_node = NULL;
	if(m_treeview != NULL)
		selected_node = m_treeview->getSelected();

	if(event.EventType==EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown)
	{
		switch (event.KeyInput.Key) {
		case KEY_ESCAPE: {
			quitMenu();
			return true;
		}
		//	irrlicht's built-in TreeView gui has no keyboard control,
		//	so we do it here: up/down to select prev/next node,
		//	left/right to collapse/expand nodes, space to toggle
		//	enabled/disabled.
		case KEY_DOWN: {
			if(selected_node != NULL)
			{
				gui::IGUITreeViewNode* node = selected_node->getNextVisible();
				if(node != NULL)
				{
					node->setSelected(true);
					adjustSidebar();
				}
			}
			return true;
		}
		case KEY_UP: {
			if(selected_node != NULL)
			{
				gui::IGUITreeViewNode* node = selected_node->getPrevSibling();
				if(node!=NULL)
				{
					node->setSelected(true);
					adjustSidebar();
				}
				else 
				{
					gui::IGUITreeViewNode* parent = selected_node->getParent();
					if(selected_node == parent->getFirstChild() &&
					   parent != m_treeview->getRoot()) 
					{
						parent->setSelected(true);
						adjustSidebar();
					}
				}
			}
			return true;
		}
		case KEY_RIGHT: {
			if(selected_node != NULL && selected_node->hasChilds())
				selected_node->setExpanded(true);
			return true;
		}
		case KEY_LEFT: {
			if(selected_node != NULL && selected_node->hasChilds())
				selected_node->setExpanded(false);
			return true;
		}
		case KEY_SPACE: {
			if(selected_node != NULL && !selected_node->hasChilds() && 
			   selected_node->getText() != NULL)
			{
				std::string modname = wide_to_narrow(selected_node->getText());
				bool checked = m_enabled_checkbox->isChecked();
				m_enabled_checkbox->setChecked(!checked);
				setEnabled(modname,!checked);
			}
			return true;
		}
		default: {}
		}
	}
	if(event.EventType==EET_GUI_EVENT)
	{
		if(event.GUIEvent.EventType==gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible())
		{
			if(!canTakeFocus(event.GUIEvent.Element))
			{
				dstream<<"GUIConfigureWorld: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED){
			switch(event.GUIEvent.Caller->getID()){
			case GUI_ID_CANCEL: {
				quitMenu();
				return true;
			}
			case GUI_ID_SAVE: {
				for(std::set<std::string>::iterator it = m_new_mod_names.begin();
					it!= m_new_mod_names.end(); ++it)
				{
					m_settings.setBool("load_mod_"+(*it),true);
				}
				std::string worldmtfile = m_wspec.path+DIR_DELIM+"world.mt";
				m_settings.updateConfigFile(worldmtfile.c_str());

				// The trailing spaces are because there seems to be a
				// bug in the text-size calculation. if the trailing
				// spaces are removed from the message text, the
				// message gets wrapped and parts of it are cut off:
				wchar_t* text = wgettext("Configuration saved.  ");
				GUIMessageMenu *menu = 
					new GUIMessageMenu(Environment, Parent, -1, m_menumgr, 
									 text );
				delete[] text;
				menu->drop();

				try
				{
					ModConfiguration modconf(m_wspec.path);
					if(!modconf.isConsistent())
					{
						wchar_t* text = wgettext("Warning: Configuration not consistent.  ");
						GUIMessageMenu *menu =
							new GUIMessageMenu(Environment, Parent, -1, m_menumgr, 
										 text );
						delete[] text;
						menu->drop();
					}
				}
				catch(ModError &err)
				{
					errorstream<<err.what()<<std::endl;
					std::wstring text = narrow_to_wide(err.what()) + wgettext("\nCheck debug.txt for details.");
					GUIMessageMenu *menu =
						new GUIMessageMenu(Environment, Parent, -1, m_menumgr, 
									 text );
					menu->drop();
				}

				quitMenu();	
				return true;
			}
			case GUI_ID_ENABLEALL: {
				if(selected_node != NULL && selected_node->getParent() == m_treeview->getRoot())
				{  
					enableAllMods(m_addonmods,true);
				} 
				else if(selected_node != NULL && selected_node->getText() != NULL)
				{  
					std::string modname = wide_to_narrow(selected_node->getText());
					std::map<std::string, ModSpec>::iterator mod_it = m_addonmods.find(modname);
					if(mod_it != m_addonmods.end())
						enableAllMods(mod_it->second.modpack_content,true);
				}
				return true;
			}
			case GUI_ID_DISABLEALL: {
				if(selected_node != NULL && selected_node->getParent() == m_treeview->getRoot())
				{
					enableAllMods(m_addonmods,false);
				} 
				if(selected_node != NULL && selected_node->getText() != NULL)
				{
					std::string modname = wide_to_narrow(selected_node->getText());
					std::map<std::string, ModSpec>::iterator mod_it = m_addonmods.find(modname);
					if(mod_it != m_addonmods.end())
						enableAllMods(mod_it->second.modpack_content,false);
				}
				return true;
			}
			}
		}	
		if(event.GUIEvent.EventType==gui::EGET_CHECKBOX_CHANGED &&
			event.GUIEvent.Caller->getID() == GUI_ID_ENABLED_CHECKBOX)
		{
			if(selected_node != NULL && !selected_node->hasChilds() && 
			   selected_node->getText() != NULL)
			{
				std::string modname = wide_to_narrow(selected_node->getText());
				setEnabled(modname, m_enabled_checkbox->isChecked());
			}
			return true;
		}
		if(event.GUIEvent.EventType==gui::EGET_TREEVIEW_NODE_SELECT &&
		   event.GUIEvent.Caller->getID() == GUI_ID_MOD_TREEVIEW)
		{
			selecting_dep = -1;
			selecting_rdep = -1;
			adjustSidebar();
			return true;
		}
		if(event.GUIEvent.EventType==gui::EGET_LISTBOX_CHANGED &&
		   event.GUIEvent.Caller->getID() == GUI_ID_DEPENDS_LISTBOX)
		{
			selecting_dep = m_dependencies_listbox->getSelected();
			selecting_rdep = -1;
			return true;
		}
		if(event.GUIEvent.EventType==gui::EGET_LISTBOX_CHANGED &&
		   event.GUIEvent.Caller->getID() == GUI_ID_RDEPENDS_LISTBOX)
		{
			selecting_dep = -1;
			selecting_rdep = m_rdependencies_listbox->getSelected();
			return true;
		}

		//double click in a dependency listbox: find corresponding
		//treeviewnode and select it:
		if(event.GUIEvent.EventType==gui::EGET_LISTBOX_SELECTED_AGAIN)
		{
			gui::IGUIListBox* box = NULL;
			if(event.GUIEvent.Caller->getID() == GUI_ID_DEPENDS_LISTBOX)
			{
				box = m_dependencies_listbox;
				if(box->getSelected() != selecting_dep)
					return true;
			}
			if(event.GUIEvent.Caller->getID() == GUI_ID_RDEPENDS_LISTBOX)
			{
				box = m_rdependencies_listbox;
				if(box->getSelected() != selecting_rdep)
					return true;
			}
			if(box != NULL && box->getSelected() != -1 &&
			   box->getListItem(box->getSelected()) != NULL)
			{
				std::string modname = 
					wide_to_narrow(box->getListItem(box->getSelected()));
				std::map<std::string, gui::IGUITreeViewNode*>::iterator it =
					m_nodes.find(modname);
				if(it != m_nodes.end())
				{
					// select node and make sure node is visible by
					// expanding all parents
					gui::IGUITreeViewNode* node = (*it).second;
					node->setSelected(true);
					while(!node->isVisible() && 
						  node->getParent() != m_treeview->getRoot())
					{
						node = node->getParent();
						node->setExpanded(true);
					}
					adjustSidebar();
				}
			}
			return true;
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

void GUIConfigureWorld::buildTreeView(std::map<std::string, ModSpec> mods, 
									  gui::IGUITreeViewNode* node)
{
	for(std::map<std::string,ModSpec>::iterator it = mods.begin();
		it != mods.end(); ++it)    
	{
		std::string modname = (*it).first;
		ModSpec mod = (*it).second;
		gui::IGUITreeViewNode* new_node = 
			node->addChildBack(narrow_to_wide(modname).c_str());
		m_nodes.insert(std::make_pair(modname, new_node));
		if(mod.is_modpack)
			buildTreeView(mod.modpack_content, new_node);
		else
		{
			// set icon for node: ? for new mods, x for disabled mods,
			// checkmark for enabled mods
			if(m_new_mod_names.count(modname) > 0)
			{
				new_node->setIcon(QUESTIONMARK_STR);
			}
			else
			{
				bool mod_enabled = true;
				if(m_settings.exists("load_mod_"+modname))
					mod_enabled = m_settings.getBool("load_mod_"+modname);
				if(mod_enabled)
					new_node->setIcon(CHECKMARK_STR);
				else 
					new_node->setIcon(CROSS_STR);
			}
		}
	}
}


void GUIConfigureWorld::adjustSidebar()
{
	gui::IGUITreeViewNode* node = m_treeview->getSelected();
	std::wstring modname_w;
	if(node->getText() != NULL)
		modname_w = node->getText();
	else 
		modname_w = L"N/A";
	std::string modname = wide_to_narrow(modname_w);

	ModSpec mspec;
	std::map<std::string, ModSpec>::iterator it = m_addonmods.find(modname);
	if(it != m_addonmods.end())
		mspec = it->second;

	m_dependencies_listbox->clear();
	m_rdependencies_listbox->clear();

	// if no mods installed, there is nothing to enable/disable, so we
	// don't show buttons or checkbox on the sidebar
	if(node->getParent() == m_treeview->getRoot() && !node->hasChilds())
	{
		m_disableall->setVisible(false);
		m_enableall->setVisible(false);
		m_enabled_checkbox->setVisible(false);
		return;
	} 
	
    // a modpack is not enabled/disabled by itself, only its cotnents
    // are. so we show show enable/disable all buttons, but hide the
    // checkbox
	if(node->getParent() == m_treeview->getRoot() ||
	   mspec.is_modpack)
	{
		m_enabled_checkbox->setVisible(false);
		m_disableall->setVisible(true);
		m_enableall->setVisible(true);
		m_modname_text->setText((L"Modpack: "+modname_w).c_str());
		return;
	}	

	// for a normal mod, we hide the enable/disable all buttons, but show the checkbox.
	m_disableall->setVisible(false);
	m_enableall->setVisible(false);
	m_enabled_checkbox->setVisible(true);
	m_modname_text->setText((L"Mod: "+modname_w).c_str());

	// the mod is enabled unless it is disabled in the world.mt settings. 
	bool mod_enabled = true;
	if(m_settings.exists("load_mod_"+modname))
		mod_enabled = m_settings.getBool("load_mod_"+modname);
	m_enabled_checkbox->setChecked(mod_enabled);

	for(std::set<std::string>::iterator it=mspec.depends.begin();
		it != mspec.depends.end(); ++it)
	{
		// check if it is an add-on mod or a game/world mod. We only
		// want to show add-ons
		std::string dependency = (*it);
		if(m_gamemods.count(dependency) > 0)
			dependency += " (" + m_gspec.id + ")";
		else if(m_worldmods.count(dependency) > 0)
			dependency += " (" + m_wspec.name + ")";
		else if(m_addonmods.count(dependency) == 0)
			dependency += " (missing)";
		m_dependencies_listbox->addItem(narrow_to_wide(dependency).c_str());
	}

	// reverse dependencies of this mod:
	std::pair< std::multimap<std::string, std::string>::iterator, 
			std::multimap<std::string, std::string>::iterator > rdep = 
		m_reverse_depends.equal_range(modname);
	for(std::multimap<std::string,std::string>::iterator it = rdep.first;
		it != rdep.second; ++it)
	{
		// check if it is an add-on mod or a game/world mod. We only
		// want to show add-ons
		std::string rdependency = (*it).second;
		if(m_addonmods.count(rdependency) > 0)
			m_rdependencies_listbox->addItem(narrow_to_wide(rdependency).c_str());
	}
}

void GUIConfigureWorld::enableAllMods(std::map<std::string, ModSpec> mods,bool enable)
{
	for(std::map<std::string, ModSpec>::iterator it = mods.begin();
		it != mods.end(); ++it)
	{
		ModSpec mod = (*it).second;
		if(mod.is_modpack) 
			// a modpack, recursively enable all mods in it
			enableAllMods(mod.modpack_content,enable);
		else // not a modpack
			setEnabled(mod.name, enable);

	}
}

void GUIConfigureWorld::enableMod(std::string modname)
{
	std::map<std::string, ModSpec>::iterator mod_it = m_addonmods.find(modname);
	if(mod_it == m_addonmods.end()){
		errorstream << "enableMod() called with invalid mod name \"" << modname << "\"" << std::endl;
		return;
	}
	ModSpec mspec = mod_it->second;
	m_settings.setBool("load_mod_"+modname,true);
	std::map<std::string,gui::IGUITreeViewNode*>::iterator it = 
		m_nodes.find(modname);
	if(it != m_nodes.end())
		(*it).second->setIcon(CHECKMARK_STR);
	m_new_mod_names.erase(modname);
	//also enable all dependencies
	for(std::set<std::string>::iterator it=mspec.depends.begin();
		it != mspec.depends.end(); ++it)
	{
		std::string dependency = *it;
		// only enable it if it is an add-on mod
		if(m_addonmods.count(dependency) > 0)
			enableMod(dependency);
	}
}

void GUIConfigureWorld::disableMod(std::string modname)
{
	std::map<std::string, ModSpec>::iterator mod_it = m_addonmods.find(modname);
	if(mod_it == m_addonmods.end()){
		errorstream << "disableMod() called with invalid mod name \"" << modname << "\"" << std::endl;
		return;
	}

	m_settings.setBool("load_mod_"+modname,false);
	std::map<std::string,gui::IGUITreeViewNode*>::iterator it = 
		m_nodes.find(modname);
 	if(it != m_nodes.end())
		(*it).second->setIcon(CROSS_STR);
	m_new_mod_names.erase(modname);
	//also disable all mods that depend on this one
	std::pair<std::multimap<std::string, std::string>::iterator, 
			  std::multimap<std::string, std::string>::iterator > rdep = 
		m_reverse_depends.equal_range(modname);
	for(std::multimap<std::string,std::string>::iterator it = rdep.first;
		it != rdep.second; ++it)
	{
		std::string rdependency = (*it).second;
		// only disable it if it is an add-on mod
		if(m_addonmods.count(rdependency) > 0)
			disableMod(rdependency);
	}	
}


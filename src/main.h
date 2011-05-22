/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef MAIN_HEADER
#define MAIN_HEADER

// Settings
#include "utility.h"
extern Settings g_settings;

// This makes and maps textures
#include "tile.h"
extern ITextureSource *g_texturesource;

// Debug streams

#include <fstream>

extern std::ostream *dout_con_ptr;
extern std::ostream *derr_con_ptr;
extern std::ostream *dout_client_ptr;
extern std::ostream *derr_client_ptr;
extern std::ostream *dout_server_ptr;
extern std::ostream *derr_server_ptr;

#define dout_con (*dout_con_ptr)
#define derr_con (*derr_con_ptr)
#define dout_client (*dout_client_ptr)
#define derr_client (*derr_client_ptr)
#define dout_server (*dout_server_ptr)
#define derr_server (*derr_server_ptr)

/*
	All kinds of stuff that needs to be exposed from main.cpp
*/

#include "modalMenu.h"
#include "guiPauseMenu.h" //For IGameCallback

extern gui::IGUIEnvironment* guienv;
extern gui::IGUIStaticText *guiroot;

// Handler for the modal menus

class MainMenuManager : public IMenuManager
{
public:
	virtual void createdMenu(GUIModalMenu *menu)
	{
		for(core::list<GUIModalMenu*>::Iterator
				i = m_stack.begin();
				i != m_stack.end(); i++)
		{
			assert(*i != menu);
		}

		if(m_stack.size() != 0)
			(*m_stack.getLast())->setVisible(false);
		m_stack.push_back(menu);
	}

	virtual void deletingMenu(GUIModalMenu *menu)
	{
		// Remove all entries if there are duplicates
		bool removed_entry;
		do{
			removed_entry = false;
			for(core::list<GUIModalMenu*>::Iterator
					i = m_stack.begin();
					i != m_stack.end(); i++)
			{
				if(*i == menu)
				{
					m_stack.erase(i);
					removed_entry = true;
					break;
				}
			}
		}while(removed_entry);

		/*core::list<GUIModalMenu*>::Iterator i = m_stack.getLast();
		assert(*i == menu);
		m_stack.erase(i);*/
		
		if(m_stack.size() != 0)
			(*m_stack.getLast())->setVisible(true);
	}

	u32 menuCount()
	{
		return m_stack.size();
	}

	core::list<GUIModalMenu*> m_stack;
};

extern MainMenuManager g_menumgr;

extern bool noMenuActive();

class MainGameCallback : public IGameCallback
{
public:
	MainGameCallback(IrrlichtDevice *a_device):
		disconnect_requested(false),
		changepassword_requested(false),
		device(a_device)
	{
	}

	virtual void exitToOS()
	{
		device->closeDevice();
	}

	virtual void disconnect()
	{
		disconnect_requested = true;
	}

	virtual void changePassword()
	{
		changepassword_requested = true;
	}

	bool disconnect_requested;
	bool changepassword_requested;
	IrrlichtDevice *device;
};

extern MainGameCallback *g_gamecallback;

#endif


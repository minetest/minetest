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

#ifndef GUIFURNACEMENU_HEADER
#define GUIFURNACEMENU_HEADER

#include "guiInventoryMenu.h"

class Client;

class GUIFurnaceMenu : public GUIInventoryMenu
{
public:
	GUIFurnaceMenu(
			gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr,
			v3s16 nodepos,
			Client *client
			);
private:
	
	v3s16 m_nodepos;
	Client *m_client;
};

#endif


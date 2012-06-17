/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef GUIPAUSEMENU_HEADER
#define GUIPAUSEMENU_HEADER

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"

class IGameCallback
{
public:
	virtual void exitToOS() = 0;
	virtual void disconnect() = 0;
	virtual void changePassword() = 0;
};

class GUIPauseMenu : public GUIModalMenu
{
public:
	GUIPauseMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IGameCallback *gamecallback,
			IMenuManager *menumgr,
			bool simple_singleplayer_mode);
	~GUIPauseMenu();
	
	void removeChildren();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	bool OnEvent(const SEvent& event);
	
private:
	IGameCallback *m_gamecallback;
	bool m_simple_singleplayer_mode;
};

#endif


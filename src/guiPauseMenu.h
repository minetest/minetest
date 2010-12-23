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

#ifndef GUIPAUSEMENU_HEADER
#define GUIPAUSEMENU_HEADER

#include "common_irrlicht.h"
#include "modalMenu.h"

class GUIPauseMenu : public GUIModalMenu
{
public:
	GUIPauseMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IrrlichtDevice *dev,
			int *active_menu_count);
	~GUIPauseMenu();
	
	void removeChildren();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	bool OnEvent(const SEvent& event);
	
private:
	IrrlichtDevice *m_dev;
};

#endif


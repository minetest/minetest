/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef GUICREATEWORLD_HEADER
#define GUICREATEWORLD_HEADER

#include "irrlichttypes_extrabloated.h"
#include "modalMenu.h"
#include <string>
#include "subgame.h"

struct CreateWorldDest
{
	virtual void accepted(std::wstring name, std::string gameid) = 0;
	virtual ~CreateWorldDest() {};
};

class GUICreateWorld : public GUIModalMenu
{
public:
	GUICreateWorld(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr,
			CreateWorldDest *dest,
			const std::vector<SubgameSpec> &games);
	~GUICreateWorld();
	
	void removeChildren();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	void drawMenu();

	void acceptInput();

	bool OnEvent(const SEvent& event);
	
private:
	CreateWorldDest *m_dest;
	std::vector<SubgameSpec> m_games;
};

#endif


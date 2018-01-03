/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "IGUIEnvironment.h"

using namespace irr;

class GameUI
{
	// Temporary between coding time to move things here
	friend class Game;

public:
	// Flags that can, or may, change during main game loop
	struct Flags
	{
		bool show_chat;
		bool show_hud;
		bool show_minimap;
		bool force_fog_off;
		bool show_debug;
		bool show_profiler_graph;
		bool disable_camera_update;
	};

	void initFlags();
	const Flags &getFlags() const { return m_flags; }

	void showMinimap(const bool show);

private:
	Flags m_flags;

	// @TODO future move
	//	gui::IGUIStaticText *m_guitext;          // First line of debug text
	//	gui::IGUIStaticText *m_guitext2;         // Second line of debug text
	//	gui::IGUIStaticText *m_guitext_info;     // At the middle of the screen
	//	gui::IGUIStaticText *m_guitext_status;
	//	gui::IGUIStaticText *m_guitext_chat;	 // Chat text
	//	gui::IGUIStaticText *m_guitext_profiler; // Profiler text
};

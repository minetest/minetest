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

#include "gameui.h"
#include "settings.h"

void GameUI::initFlags()
{
	memset(&m_flags, 0, sizeof(GameUI::Flags));
	m_flags.show_chat = true;
	m_flags.show_hud = true;
	m_flags.show_debug = g_settings->getBool("show_debug");
}

void GameUI::showMinimap(const bool show)
{
	m_flags.show_minimap = show;
}

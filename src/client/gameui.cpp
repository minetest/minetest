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
#include <irrlicht_changes/static_text.h>
#include "gui/mainmenumanager.h"
#include "client.h"
#include "fontengine.h"
#include "clientmap.h"
#include "version.h"
#include "renderingengine.h"

void GameUI::init()
{
	// First line of debug text
	m_guitext = gui::StaticText::add(guienv, utf8_to_wide(PROJECT_NAME_C).c_str(),
		core::rect<s32>(0, 0, 0, 0), false, false, guiroot);
}

void GameUI::update(const RunStats &stats, Client *client,
	const MapDrawControl *draw_control)
{
	if (m_flags.show_debug) {
		static float drawtime_avg = 0;
		drawtime_avg = drawtime_avg * 0.95 + stats.drawtime * 0.05;
		u16 fps = 1.0 / stats.dtime_jitter.avg;

		std::ostringstream os(std::ios_base::binary);
		os << std::fixed
			<< PROJECT_NAME_C " " << g_version_hash
			<< ", FPS: " << fps
			<< std::setprecision(0)
			<< ", drawtime: " << drawtime_avg << "ms"
			<< std::setprecision(1)
			<< ", dtime jitter: "
			<< (stats.dtime_jitter.max_fraction * 100.0) << "%"
			<< std::setprecision(1)
			<< ", view range: "
			<< (draw_control->range_all ? "All" : itos(draw_control->wanted_range))
			<< std::setprecision(3)
			<< ", RTT: " << client->getRTT() << "s";
		setStaticText(m_guitext, utf8_to_wide(os.str()).c_str());
		m_guitext->setVisible(true);
	} else {
		m_guitext->setVisible(false);
	}

	if (m_guitext->isVisible()) {
		v2u32 screensize = RenderingEngine::get_instance()->getWindowSize();
		m_guitext->setRelativePosition(core::rect<s32>(5, 5, screensize.X,
			5 + g_fontengine->getTextHeight()));
	}
}

void GameUI::initFlags()
{
	memset(&m_flags, 0, sizeof(GameUI::Flags));
	m_flags.show_chat = true;
	m_flags.show_hud = true;
	m_flags.show_debug = g_settings->getBool("show_debug");
}

void GameUI::showMinimap(bool show)
{
	m_flags.show_minimap = show;
}

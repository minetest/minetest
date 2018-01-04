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
#include "util/pointedthing.h"
#include "client.h"
#include "clientmap.h"
#include "fontengine.h"
#include "nodedef.h"
#include "renderingengine.h"
#include "version.h"

inline static const char *yawToDirectionString(int yaw)
{
	static const char *direction[4] = {"N +Z", "W -X", "S -Z", "E +X"};

	yaw = wrapDegrees_0_360(yaw);
	yaw = (yaw + 45) % 360 / 90;

	return direction[yaw];
}

void GameUI::init()
{
	// First line of debug text
	m_guitext = gui::StaticText::add(guienv, utf8_to_wide(PROJECT_NAME_C).c_str(),
		core::rect<s32>(0, 0, 0, 0), false, false, guiroot);

	// Second line of debug text
	m_guitext2 = gui::StaticText::add(guienv, L"", core::rect<s32>(0, 0, 0, 0), false,
		false, guiroot);

	// At the middle of the screen
	// Object infos are shown in this
	m_guitext_info = gui::StaticText::add(guienv, L"",
		core::rect<s32>(0, 0, 400, g_fontengine->getTextHeight() * 5 + 5)
			+ v2s32(100, 200), false, true, guiroot);

	// Status text (displays info when showing and hiding GUI stuff, etc.)
	m_guitext_status = gui::StaticText::add(guienv, L"<Status>",
		core::rect<s32>(0, 0, 0, 0), false, false, guiroot);

	m_guitext_status->setVisible(false);
}

void GameUI::update(const RunStats &stats, Client *client, MapDrawControl *draw_control,
	const CameraOrientation &cam, const PointedThing &pointed_old, float dtime)
{
	v2u32 screensize = RenderingEngine::get_instance()->getWindowSize();

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

		m_guitext->setRelativePosition(core::rect<s32>(5, 5, screensize.X,
			5 + g_fontengine->getTextHeight()));
	}

	// Finally set the guitext visible depending on the flag
	m_guitext->setVisible(m_flags.show_debug);

	if (m_flags.show_debug) {
		LocalPlayer *player = client->getEnv().getLocalPlayer();
		v3f player_position = player->getPosition();

		std::ostringstream os(std::ios_base::binary);
		os << std::setprecision(1) << std::fixed
			<< "pos: (" << (player_position.X / BS)
			<< ", " << (player_position.Y / BS)
			<< ", " << (player_position.Z / BS)
			<< "), yaw: " << (wrapDegrees_0_360(cam.camera_yaw)) << "° "
			<< yawToDirectionString(cam.camera_yaw)
			<< ", seed: " << ((u64)client->getMapSeed());

		if (pointed_old.type == POINTEDTHING_NODE) {
			ClientMap &map = client->getEnv().getClientMap();
			const INodeDefManager *nodedef = client->getNodeDefManager();
			MapNode n = map.getNodeNoEx(pointed_old.node_undersurface);

			if (n.getContent() != CONTENT_IGNORE && nodedef->get(n).name != "unknown") {
				os << ", pointed: " << nodedef->get(n).name
					<< ", param2: " << (u64) n.getParam2();
			}
		}

		setStaticText(m_guitext2, utf8_to_wide(os.str()).c_str());

		m_guitext2->setRelativePosition(core::rect<s32>(5,
			5 + g_fontengine->getTextHeight(), screensize.X,
			5 + g_fontengine->getTextHeight() * 2
		));
	}

	m_guitext2->setVisible(m_flags.show_debug);

	setStaticText(m_guitext_info, translate_string(m_infotext).c_str());
	m_guitext_info->setVisible(m_flags.show_hud && g_menumgr.menuCount() == 0);

	static const float statustext_time_max = 1.5f;

	if (!m_statustext.empty()) {
		m_statustext_time += dtime;

		if (m_statustext_time >= statustext_time_max) {
			clearStatusText();
			m_statustext_time = 0.0f;
		}
	}

	setStaticText(m_guitext_status, translate_string(m_statustext).c_str());
	m_guitext_status->setVisible(!m_statustext.empty());

	if (!m_statustext.empty()) {
		s32 status_width  = m_guitext_status->getTextWidth();
		s32 status_height = m_guitext_status->getTextHeight();
		s32 status_y = screensize.Y - 150;
		s32 status_x = (screensize.X - status_width) / 2;

		m_guitext_status->setRelativePosition(core::rect<s32>(status_x ,
			status_y - status_height, status_x + status_width, status_y));

		// Fade out
		video::SColor initial_color(255, 0, 0, 0);

		if (guienv->getSkin())
			initial_color = guienv->getSkin()->getColor(gui::EGDC_BUTTON_TEXT);

		video::SColor final_color = initial_color;
		final_color.setAlpha(0);
		video::SColor fade_color = initial_color.getInterpolated_quadratic(
			initial_color, final_color,
			pow(m_statustext_time / statustext_time_max, 2.0f));
		m_guitext_status->setOverrideColor(fade_color);
		m_guitext_status->enableOverrideColor(true);
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

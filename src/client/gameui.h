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

#include <IGUIEnvironment.h>
#include "util/enriched_string.h"
#include "util/pointedthing.h"
#include "game.h"

using namespace irr;
class Client;
struct MapDrawControl;

class GameUI
{
	// Temporary between coding time to move things here
	friend class Game;

	// Permit unittests to access members directly
	friend class TestGameUI;

public:
	GameUI() = default;
	~GameUI() = default;

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

	void init();
	void update(const RunStats &stats, Client *client, MapDrawControl *draw_control,
			const CameraOrientation &cam, const PointedThing &pointed_old,
			float dtime);

	void initFlags();
	const Flags &getFlags() const { return m_flags; }

	void showMinimap(bool show);

	inline void setInfoText(const std::wstring &str) { m_infotext = str; }
	inline void clearInfoText() { m_infotext.clear(); }

	inline void showStatusText(const std::wstring &str)
	{
		m_statustext = str;
		m_statustext_time = 0.0f;
	}
	void showTranslatedStatusText(const char *str);
	inline void clearStatusText() { m_statustext.clear(); }

	void setChatText(const EnrichedString &chat_text, u32 recent_chat_count,
			u32 profiler_current_page);

	void updateProfiler(u32 profiler_current_page, u32 profiler_max_page);

private:
	Flags m_flags;

	gui::IGUIStaticText *m_guitext = nullptr; // First line of debug text
	gui::IGUIStaticText *m_guitext2 = nullptr; // Second line of debug text

	gui::IGUIStaticText *m_guitext_info = nullptr; // At the middle of the screen
	std::wstring m_infotext;

	gui::IGUIStaticText *m_guitext_status = nullptr;
	std::wstring m_statustext;
	float m_statustext_time = 0.0f;

	gui::IGUIStaticText *m_guitext_chat; // Chat text
	gui::IGUIStaticText *m_guitext_profiler; // Profiler text
};

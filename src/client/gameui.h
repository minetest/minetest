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

#include "irrlichttypes.h"
#include <IGUIEnvironment.h>
#include "gui/guiFormSpecMenu.h"
#include "util/enriched_string.h"
#include "util/pointedthing.h"
#include "game.h"

using namespace irr;
class Client;
class GUIChatConsole;
struct MapDrawControl;

/*
 * This object intend to contain the core UI elements
 * It includes:
 *   - status texts
 *   - debug texts
 *   - chat texts
 *   - hud flags
 */
class GameUI
{
	// Temporary between coding time to move things here
	friend class Game;

	// Permit unittests to access members directly
	friend class TestGameUI;

public:
	GameUI();
	~GameUI() = default;

	// Flags that can, or may, change during main game loop
	struct Flags
	{
		bool show_chat = true;
		bool show_hud = true;
		bool show_minimap = false;
		bool show_minimal_debug = false;
		bool show_basic_debug = false;
		bool show_profiler_graph = false;
	};

	void init();
	void update(const RunStats &stats, Client *client, MapDrawControl *draw_control,
			const CameraOrientation &cam, const PointedThing &pointed_old,
			const GUIChatConsole *chat_console, float dtime);

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

	bool isChatVisible()
	{
		return m_flags.show_chat && m_recent_chat_count != 0 && m_profiler_current_page == 0;
	}
	void setChatText(const EnrichedString &chat_text, u32 recent_chat_count);
	void updateChatSize();

	void updateProfiler();

	void toggleChat();
	void toggleHud();
	void toggleProfiler();

	GUIFormSpecMenu *&updateFormspec(const std::string &formname)
	{
		m_formname = formname;
		return m_formspec;
	}

	const std::string &getFormspecName() { return m_formname; }
	GUIFormSpecMenu *&getFormspecGUI() { return m_formspec; }
	void deleteFormspec();

private:
	Flags m_flags;

	float m_drawtime_avg = 0;

	gui::IGUIStaticText *m_guitext = nullptr;  // First line of debug text
	gui::IGUIStaticText *m_guitext2 = nullptr; // Second line of debug text

	gui::IGUIStaticText *m_guitext_info = nullptr; // At the middle of the screen
	std::wstring m_infotext;

	gui::IGUIStaticText *m_guitext_status = nullptr;
	std::wstring m_statustext;
	float m_statustext_time = 0.0f;
	video::SColor m_statustext_initial_color;

	gui::IGUIStaticText *m_guitext_chat = nullptr; // Chat text
	u32 m_recent_chat_count = 0;
	core::rect<s32> m_current_chat_size{0, 0, 0, 0};

	gui::IGUIStaticText *m_guitext_profiler = nullptr; // Profiler text
	u8 m_profiler_current_page = 0;
	const u8 m_profiler_max_page = 3;

	// Default: "". If other than "": Empty show_formspec packets will only
	// close the formspec when the formname matches
	std::string m_formname;
	GUIFormSpecMenu *m_formspec = nullptr;
};

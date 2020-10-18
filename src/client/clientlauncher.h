/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes_extrabloated.h"
#include "client/inputhandler.h"
#include "gameparams.h"

class RenderingEngine;

class ClientLauncher
{
public:
	ClientLauncher() = default;

	~ClientLauncher();

	bool run(GameStartData &start_data, const Settings &cmd_args);

private:
	void init_args(GameStartData &start_data, const Settings &cmd_args);
	bool init_engine();
	void init_input();

	bool launch_game(std::string &error_message, bool reconnect_requested,
		GameStartData &start_data, const Settings &cmd_args);

	void main_menu(MainMenuData *menudata);

	void speed_tests();

	bool list_video_modes = false;
	bool skip_main_menu = false;
	bool random_input = false;
	InputHandler *input = nullptr;
	MyEventReceiver *receiver = nullptr;
	gui::IGUISkin *skin = nullptr;
};

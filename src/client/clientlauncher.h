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

	bool run(GameParams &game_params, const Settings &cmd_args);

protected:
	void init_args(GameParams &game_params, const Settings &cmd_args);
	bool init_engine();
	void init_input();

	bool launch_game(std::string &error_message, bool reconnect_requested,
		GameParams &game_params, const Settings &cmd_args);

	void main_menu(MainMenuData *menudata);

	void speed_tests();

	bool list_video_modes = false;
	bool skip_main_menu = false;
	bool use_freetype = false;
	bool random_input = false;
	std::string address = "";
	std::string playername = "";
	std::string password = "";
	InputHandler *input = nullptr;
	MyEventReceiver *receiver = nullptr;
	gui::IGUISkin *skin = nullptr;
	gui::IGUIFont *font = nullptr;
	SubgameSpec gamespec;
	WorldSpec worldspec;
	bool simple_singleplayer_mode = false;

	// These are set up based on the menu and other things
	// TODO: Are these required since there's already playername, password, etc
	std::string current_playername = "inv£lid";
	std::string current_password = "";
	std::string current_address = "does-not-exist";
	int current_port = 0;
};

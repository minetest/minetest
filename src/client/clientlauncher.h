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

#ifndef __CLIENT_LAUNCHER_H__
#define __CLIENT_LAUNCHER_H__

#include "irrlichttypes_extrabloated.h"
#include "client/inputhandler.h"
#include "gameparams.h"

// A small helper class
class TimeGetter
{
public:
	virtual u32 getTime(TimePrecision prec) = 0;
};

// A precise irrlicht one
class IrrlichtTimeGetter: public TimeGetter
{
public:
	IrrlichtTimeGetter(IrrlichtDevice *device):
		m_device(device)
	{}
	u32 getTime(TimePrecision prec)
	{
		if (prec == PRECISION_MILLI) {
			if (m_device == NULL)
				return 0;
			return m_device->getTimer()->getRealTime();
		} else {
			return porting::getTime(prec);
		}
	}
private:
	IrrlichtDevice *m_device;
};
// Not so precise one which works without irrlicht
class SimpleTimeGetter: public TimeGetter
{
public:
	u32 getTime(TimePrecision prec)
	{
		return porting::getTime(prec);
	}
};

class ClientLauncher
{
public:
	ClientLauncher() :
		list_video_modes(false),
		skip_main_menu(false),
		use_freetype(false),
		random_input(false),
		address(""),
		playername(""),
		password(""),
		device(NULL),
		input(NULL),
		receiver(NULL),
		skin(NULL),
		font(NULL),
		simple_singleplayer_mode(false),
		current_playername("inv£lid"),
		current_password(""),
		current_address("does-not-exist"),
		current_port(0)
	{}

	~ClientLauncher();

	bool run(GameParams &game_params, const Settings &cmd_args);

protected:
	void init_args(GameParams &game_params, const Settings &cmd_args);
	bool init_engine();
	void init_input();

	bool launch_game(std::string &error_message, bool reconnect_requested,
		GameParams &game_params, const Settings &cmd_args);

	void main_menu(MainMenuData *menudata);
	bool create_engine_device();

	void speed_tests();
	bool print_video_modes();

	bool list_video_modes;
	bool skip_main_menu;
	bool use_freetype;
	bool random_input;
	std::string address;
	std::string playername;
	std::string password;
	IrrlichtDevice *device;
	InputHandler *input;
	MyEventReceiver *receiver;
	gui::IGUISkin *skin;
	gui::IGUIFont *font;
	scene::ISceneManager *smgr;
	SubgameSpec gamespec;
	WorldSpec worldspec;
	bool simple_singleplayer_mode;

	// These are set up based on the menu and other things
	// TODO: Are these required since there's already playername, password, etc
	std::string current_playername;
	std::string current_password;
	std::string current_address;
	int current_port;
};

#endif

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

#include "client/game/Game.h"
#include "game.h"
#include "gettext.h"

/****************************************************************************/
/****************************************************************************
 extern function for launching the game
 ****************************************************************************/
/****************************************************************************/

void the_game(bool *kill,
		bool random_input,
		InputHandler *input,
		const std::string &map_dir,
		const std::string &playername,
		const std::string &password,
		const std::string &address,         // If empty local server is created
		u16 port,

		std::string &error_message,
		ChatBackend &chat_backend,
		bool *reconnect_requested,
		const SubgameSpec &gamespec,        // Used for local game
		bool simple_singleplayer_mode)
{
	Game game;

	/* Make a copy of the server address because if a local singleplayer server
	 * is created then this is updated and we don't want to change the value
	 * passed to us by the calling function
	 */
	std::string server_address = address;

	try {

		if (game.startup(kill, random_input, input, map_dir,
				playername, password, &server_address, port, error_message,
				reconnect_requested, &chat_backend, gamespec,
				simple_singleplayer_mode)) {
			game.run();
			game.shutdown();
		}

	} catch (SerializationError &e) {
		error_message = std::string("A serialization error occurred:\n")
				+ e.what() + "\n\nThe server is probably "
				" running a different version of " PROJECT_NAME_C ".";
		errorstream << error_message << std::endl;
	} catch (ServerError &e) {
		error_message = e.what();
		errorstream << "ServerError: " << error_message << std::endl;
	} catch (ModError &e) {
		error_message = e.what() + strgettext("\nCheck debug.txt for details.");
		errorstream << "ModError: " << error_message << std::endl;
	}
}

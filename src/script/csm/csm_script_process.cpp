/*
Minetest
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "csm_script_process.h"
#include "csm_gamedef.h"
#include "csm_scripting.h"
#include "debug.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include <stdlib.h>
#include <string.h>

int csm_script_main(int argc, char *argv[])
{
	FILE *from_controller = fdopen(CSM_SCRIPT_READ_FD, "rb");
	FILE *to_controller = fdopen(CSM_SCRIPT_WRITE_FD, "wb");
	FATAL_ERROR_IF(!from_controller || !to_controller,
			"CSM process unable to open pipe");

	CSMGameDef gamedef(from_controller, to_controller);
	CSMScripting script(&gamedef);

	CSMRecvMsg msg = gamedef.recvEx();
	if (msg.type == CSMMsgType::C2S_RUN_LOAD_MODS) {
		{
			std::istringstream is(std::string((char *)msg.data.data(), msg.data.size()));
			msg.data.clear();
			msg.data.shrink_to_fit();
			gamedef.getWritableNodeDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
		}

		gamedef.sendEx(CSMMsgType::S2C_DONE);

		while (true) {
			msg = gamedef.recvEx();
			switch (msg.type) {
			case CSMMsgType::C2S_RUN_STEP:
				if (msg.data.size() >= sizeof(float)) {
					float dtime;
					memcpy(&dtime, msg.data.data(), sizeof(float));
					script.environment_Step(dtime);
				}
				break;
			default:
				break;
			}
			gamedef.sendEx(CSMMsgType::S2C_DONE);
		}
	}

	return 0;
}

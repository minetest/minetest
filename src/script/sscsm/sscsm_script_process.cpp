#include "sscsm_script_process.h"
#include "sscsm_gamedef.h"
#include "sscsm_scripting.h"
#include "debug.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include <stdlib.h>
#include <string.h>

int sscsm_script_main(int argc, char *argv[])
{
	FILE *from_controller = fdopen(SSCSM_SCRIPT_READ_FD, "rb");
	FILE *to_controller = fdopen(SSCSM_SCRIPT_WRITE_FD, "wb");
	FATAL_ERROR_IF(!from_controller || !to_controller,
			"SSCSM process unable to open pipe");

	SSCSMGameDef gamedef(from_controller, to_controller);
	SSCSMScripting script(&gamedef);

	SSCSMRecvMsg msg = gamedef.recvEx();
	if (msg.type == SSCSMMsgType::C2S_RUN_LOAD_MODS) {
		{
			std::istringstream is(std::string((char *)msg.data.data(), msg.data.size()));
			msg.data.clear();
			msg.data.shrink_to_fit();
			gamedef.getWritableNodeDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
		}

		gamedef.sendEx(SSCSMMsgType::S2C_DONE);

		while (true) {
			msg = gamedef.recvEx();
			switch (msg.type) {
			case SSCSMMsgType::C2S_RUN_STEP:
				if (msg.data.size() >= sizeof(float)) {
					float dtime;
					memcpy(&dtime, msg.data.data(), sizeof(float));
					script.environment_Step(dtime);
				}
				break;
			default:
				break;
			}
			gamedef.sendEx(SSCSMMsgType::S2C_DONE);
		}
	}

	return 0;
}

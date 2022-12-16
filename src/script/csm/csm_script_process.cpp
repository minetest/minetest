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

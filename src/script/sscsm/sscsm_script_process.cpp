#include "sscsm_script_process.h"
#include "sscsm_message.h"
#include "sscsm_script_api.h"
#include "debug.h"
#include "log.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include <stdlib.h>
#include <string.h>

FILE *g_sscsm_from_controller = nullptr;
FILE *g_sscsm_to_controller = nullptr;

NodeDefManager *g_sscsm_nodedef = nullptr;

int sscsm_script_main(int argc, char *argv[])
{
	g_sscsm_from_controller = fdopen(SSCSM_SCRIPT_READ_FD, "rb");
	g_sscsm_to_controller = fdopen(SSCSM_SCRIPT_WRITE_FD, "wb");
	FATAL_ERROR_IF(!g_sscsm_from_controller || !g_sscsm_to_controller,
			"SSCSM process unable to open pipe");

	lua_State *L = luaL_newstate();
	sscsm_load_libraries(L);

	SSCSMRecvMsg msg = sscsm_recv_msg_ex(g_sscsm_from_controller);
	if (msg.type == SSCSMMsgType::C2S_RUN_LOAD_MODS) {
		g_sscsm_nodedef = createNodeDefManager();
		{
			std::istringstream is(std::string((char *)msg.data.data(), msg.data.size()));
			g_sscsm_nodedef->deSerialize(is, LATEST_PROTOCOL_VERSION);
		}
		msg.data.clear();
		msg.data.shrink_to_fit();

		sscsm_load_mods(L);
		sscsm_send_msg_ex(g_sscsm_to_controller, SSCSMMsgType::S2C_DONE, 0, nullptr);

		Optional<SSCSMRecvMsg> msg;
		while ((msg = sscsm_recv_msg(g_sscsm_from_controller))) {
			switch (msg->type) {
			case SSCSMMsgType::C2S_RUN_STEP:
				if (msg->data.size() >= sizeof(float)) {
					float dtime;
					memcpy(&dtime, msg->data.data(), sizeof(float));
					sscsm_run_step(L, dtime);
				}
				break;
			default:
				break;
			}
			sscsm_send_msg_ex(g_sscsm_to_controller, SSCSMMsgType::S2C_DONE, 0, nullptr);
		}
	}

	return 0;
}

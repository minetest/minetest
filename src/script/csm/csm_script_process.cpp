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
#include "csm_message.h"
#include "csm_script_ipc.h"
#include "csm_scripting.h"
#include "cpp_api/s_base.h"
#include "debug.h"
#include "filesys.h"
#include "itemdef.h"
#include "log.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
#include "porting.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <utility>

static bool g_can_log = false;
static std::vector<std::pair<LogLevel, std::string>> g_waiting_logs;

class CSMLogOutput : public ICombinedLogOutput {
	void logRaw(LogLevel level, const std::string &line) override
	{
		g_waiting_logs.emplace_back(level, line);
		if (g_can_log) {
			std::vector<u8> send;
			for (const auto &line : g_waiting_logs) {
				CSMS2CLog log;
				log.level = line.first;
				send.resize(sizeof(log) + line.second.size());
				memcpy(send.data(), &log, sizeof(log));
				memcpy(send.data() + sizeof(log), line.second.data(), line.second.size());
				CSM_IPC(exchange(send.size(), send.data()));
			}
			g_waiting_logs.clear();
		}
	}
};

static CSMLogOutput g_log_output;

int csm_script_main(int argc, char *argv[])
{
	FATAL_ERROR_IF(argc < 4, "Too few arguments to CSM process");

	int shm = shm_open(argv[2], O_RDWR, 0);
	shm_unlink(argv[2]);
	FATAL_ERROR_IF(shm == -1, "CSM process unable to open shared memory");

	IPCChannelShared *ipc_shared = (IPCChannelShared *)mmap(nullptr, sizeof(IPCChannelShared),
			PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	FATAL_ERROR_IF(ipc_shared == MAP_FAILED, "CSM process unable to map shared memory");

	g_csm_script_ipc = IPCChannelEnd::makeB(ipc_shared);

	// TODO: only send logs if they will actually be recorded.
	g_logger.addOutput(&g_log_output);
	g_logger.registerThread("CSM");

	CSMGameDef gamedef;
	CSMScripting script(&gamedef);

	{
		std::string client_path = argv[3];
		std::string builtin_path = client_path + DIR_DELIM "csmbuiltin";
		gamedef.scanModIntoMemory(BUILTIN_MOD_NAME, builtin_path);
	}

	CSM_IPC(recv());

	g_can_log = true;

	{
		std::istringstream is(std::string((char *)csm_recv_data(), csm_recv_size()),
				std::ios::binary);
		gamedef.getWritableItemDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
		gamedef.getWritableNodeDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
	}

	script.loadModFromMemory(BUILTIN_MOD_NAME);
	script.checkSetByBuiltin();

	g_can_log = false;

	CSM_IPC(exchange(CSM_S2C_DONE));

	while (true) {
		size_t size = csm_recv_size();
		const void *data = csm_recv_data();
		CSMMsgType type = CSM_INVALID_MSG_TYPE;
		if (size >= sizeof(type))
			memcpy(&type, data, sizeof(type));
		g_can_log = true;
		switch (type) {
		case CSM_C2S_RUN_STEP:
			if (size >= sizeof(CSMC2SRunStep)) {
				CSMC2SRunStep msg;
				memcpy(&msg, data, sizeof(msg));
				script.environment_Step(msg.dtime);
			}
			break;
		default:
			break;
		}
		g_can_log = false;
		CSM_IPC(exchange(CSM_S2C_DONE));
	}

	return 0;
}

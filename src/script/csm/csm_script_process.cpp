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
#include "csm_scripting.h"
#include "debug.h"
#include "itemdef.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

IPCChannelEnd g_csm_script_ipc;

int csm_script_main(int argc, char *argv[])
{
	int shm = argc >= 3 ? shm_open(argv[2], O_RDWR, 0) : -1;
	FATAL_ERROR_IF(shm == -1, "CSM process unable to open shared memory");
	shm_unlink(argv[2]);

	IPCChannelShared *ipc_shared = (IPCChannelShared *)mmap(nullptr, sizeof(IPCChannelShared),
			PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	FATAL_ERROR_IF(ipc_shared == MAP_FAILED, "CSM process unable to map shared memory");

	g_csm_script_ipc = IPCChannelEnd::makeB(ipc_shared);

	CSMGameDef gamedef;
	CSMScripting script(&gamedef);

	CSM_IPC(recv());

	{
		std::istringstream is(std::string((char *)csm_recv_data(), csm_recv_size()),
				std::ios::binary);
		gamedef.getWritableItemDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
		gamedef.getWritableNodeDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
	}

	CSM_IPC(exchange(CSM_S2C_DONE));

	while (true) {
		size_t size = csm_recv_size();
		const void *data = csm_recv_data();
		CSMMsgType type = CSM_INVALID_MSG_TYPE;
		if (size >= sizeof(type))
			memcpy(&type, data, sizeof(type));
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
		CSM_IPC(exchange(CSM_S2C_DONE));
	}

	return 0;
}

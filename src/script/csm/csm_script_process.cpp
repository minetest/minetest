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
#include "inventory.h"
#include "itemdef.h"
#include "log.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
#include "porting.h"
#include "process_sandbox.h"
#include "util/string.h"
#include "util/serialize.h"
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}
#include <memory>
#include <stdlib.h>
#include <string.h>
#include <utility>
#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

namespace {

class CSMLogOutput : public ICombinedLogOutput {
public:
	void logRaw(LogLevel level, const std::string &line) override
	{
		if (m_can_log) {
			sendLog(level, line);
		} else {
			m_waiting_logs.emplace_back(level, line);
		}
	}

	void startLogging()
	{
		m_can_log = true;
		for (const auto &log : m_waiting_logs) {
			sendLog(log.first, log.second);
		}
		m_waiting_logs.clear();
	}

	void stopLogging()
	{
		m_can_log = false;
	}

	bool isLogging() const { return m_can_log; }

private:
	void sendLog(LogLevel level, const std::string &line)
	{
		CSMS2CLog log(CSM_S2C_LOG, level, line);
		csm_exchange_msg(log);
	}

	bool m_can_log;
	std::vector<std::pair<LogLevel, std::string>> m_waiting_logs;
};

CSMLogOutput g_log_output;

} // namespace

int csm_script_main(int argc, char *argv[])
{
#if defined(_WIN32)
	FATAL_ERROR_IF(argc < 6, "Too few arguments to CSM process");

	HANDLE shm = (HANDLE)strtoull(argv[3], nullptr, 10);
	HANDLE sem_a = (HANDLE)strtoull(argv[4], nullptr, 10);
	HANDLE sem_b = (HANDLE)strtoull(argv[5], nullptr, 10);

	IPCChannelShared *ipc_shared = (IPCChannelShared *)MapViewOfFile(shm, FILE_MAP_ALL_ACCESS,
			0, 0, sizeof(IPCChannelShared));
	FATAL_ERROR_IF(!ipc_shared, "CSM process unable to map shared memory");

	g_csm_script_ipc = IPCChannelEnd::makeB(ipc_shared, sem_a, sem_b);
#else
	FATAL_ERROR_IF(argc < 4, "Too few arguments to CSM process");

	int shm = atoi(argv[3]);
	IPCChannelShared *ipc_shared = (IPCChannelShared *)mmap(nullptr, sizeof(IPCChannelShared),
			PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	FATAL_ERROR_IF(ipc_shared == MAP_FAILED, "CSM process unable to map shared memory");

	g_csm_script_ipc = IPCChannelEnd::makeB(ipc_shared);
#endif // !defined(_WIN32)

	// TODO: only send logs if they will actually be recorded.
	g_logger.addOutput(&g_log_output);
	g_logger.registerThread("CSM");

	csm_ipc_check(g_csm_script_ipc.recv());
	auto initial_recv = csm_deserialize_msg<std::string>();

	g_log_output.startLogging();

	CSMGameDef gamedef;

	{
		std::istringstream is(std::move(initial_recv), std::ios::binary);
		gamedef.getWritableItemDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
		gamedef.getWritableNodeDefManager()->deSerialize(is, LATEST_PROTOCOL_VERSION);
	}

	CSMScripting script(&gamedef);

	std::vector<std::string> mods;

	{
		std::string client_path = argv[2];
		std::string builtin_path = client_path + DIR_DELIM "csmbuiltin";
		std::string mods_path = client_path + DIR_DELIM "csm";

		gamedef.scanModIntoMemory(BUILTIN_MOD_NAME, builtin_path);
		std::vector<fs::DirListNode> mod_dirs = fs::GetDirListing(mods_path);
		for (const fs::DirListNode &dir : mod_dirs) {
			if (dir.dir) {
				size_t number = mystoi(dir.name.substr(0, 5), 0, 99999);
				std::string name = dir.name.substr(5);
				gamedef.scanModIntoMemory(name, mods_path + DIR_DELIM + dir.name);
				mods.resize(number + 1);
				mods[number] = std::move(name);
			}
		}
	}

	if (!start_sandbox())
		warningstream << "Unable to start process sandbox" << std::endl;

	try {
		script.loadModFromMemory(BUILTIN_MOD_NAME);
		script.checkSetByBuiltin();
		for (const std::string &mod : mods) {
			if (!mod.empty())
				script.loadModFromMemory(mod);
		}

		script.on_mods_loaded();
	} catch (const std::exception &e) {
		errorstream << e.what() << std::endl;
		csm_exchange_msg(CSM_S2C_TERMINATED);
		return 0;
	}

	g_log_output.stopLogging();

	csm_exchange_msg(CSM_S2C_DONE);

	while (true) {
		size_t size = csm_recv_size();
		const void *data = csm_recv_data();
		CSMC2SMsgType type = CSM_C2S_INVALID;
		if (size >= sizeof(type))
			memcpy(&type, data, sizeof(type));
		bool sent_done = false;
		try {
			switch (type) {
			case CSM_C2S_RUN_SHUTDOWN:
				g_log_output.startLogging();
				script.on_shutdown();
				break;
			case CSM_C2S_RUN_CLIENT_READY:
				g_log_output.startLogging();
				script.on_client_ready();
				break;
			case CSM_C2S_RUN_CAMERA_READY:
				g_log_output.startLogging();
				script.on_camera_ready();
				break;
			case CSM_C2S_RUN_MINIMAP_READY:
				g_log_output.startLogging();
				script.on_minimap_ready();
				break;
			case CSM_C2S_RUN_SENDING_MESSAGE:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunSendingMessage>();
					g_log_output.startLogging();
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_sending_message(recv.second));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_RECEIVING_MESSAGE:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunReceivingMessage>();
					g_log_output.startLogging();
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_receiving_message(recv.second));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_DAMAGE_TAKEN:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunDamageTaken>();
					g_log_output.startLogging();
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_damage_taken(recv.second));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_HP_MODIFICATION:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunHPModification>();
					g_log_output.startLogging();
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_hp_modification(recv.second));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_DEATH:
				g_log_output.startLogging();
				script.on_death();
				break;
			case CSM_C2S_RUN_MODCHANNEL_MESSAGE:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunModchannelMessage>();
					g_log_output.startLogging();
					script.on_modchannel_message(std::get<1>(recv), std::get<2>(recv),
							std::get<3>(recv));
				}
				break;
			case CSM_C2S_RUN_MODCHANNEL_SIGNAL:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunModchannelSignal>();
					g_log_output.startLogging();
					script.on_modchannel_signal(std::get<1>(recv), std::get<2>(recv));
				}
				break;
			case CSM_C2S_RUN_FORMSPEC_INPUT:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunFormspecInput>();
					g_log_output.startLogging();
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_formspec_input(
							std::get<1>(recv), std::get<2>(recv)));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_INVENTORY_OPEN:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunInventoryOpen>();
					g_log_output.startLogging();
					std::istringstream is(recv.second, std::ios::binary);
					Inventory inv(gamedef.idef());
					inv.deSerialize(is);
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_inventory_open(&inv));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_ITEM_USE:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunItemUse>();
					g_log_output.startLogging();
					std::istringstream itemstring_is(std::get<1>(recv), std::ios::binary);
					ItemStack selected_item;
					selected_item.deSerialize(itemstring_is);
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_item_use(selected_item,
							std::get<2>(recv)));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_PLACENODE:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunPlacenode>();
					g_log_output.startLogging();
					script.on_placenode(std::get<1>(recv),
							gamedef.idef()->get(std::get<2>(recv)));
				}
				break;
			case CSM_C2S_RUN_PUNCHNODE:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunPunchnode>();
					g_log_output.startLogging();
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_punchnode(std::get<1>(recv),
							std::get<2>(recv)));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_DIGNODE:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunDignode>();
					g_log_output.startLogging();
					CSMS2CDoneBool send(CSM_S2C_DONE, script.on_dignode(std::get<1>(recv),
							std::get<2>(recv)));
					csm_exchange_msg(send);
					sent_done = true;
				}
				break;
			case CSM_C2S_RUN_STEP:
				{
					auto recv = csm_deserialize_msg<CSMC2SRunStep>();
					g_log_output.startLogging();
					script.environment_Step(recv.second);
				}
				break;
			default:
				break;
			}
			g_log_output.stopLogging();
		} catch (const std::exception &e) {
			if (!g_log_output.isLogging())
				throw;
			errorstream << e.what() << std::endl;
			csm_exchange_msg(CSM_S2C_TERMINATED);
			return 0;
		}
		if (!sent_done)
			csm_exchange_msg(CSM_S2C_DONE);
	}

	return 0;
}

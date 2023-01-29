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

#include "csm_controller.h"
#include "csm_message.h"
#include "client/client.h"
#include "filesys.h"
#include "inventory.h"
#include "log.h"
#include "map.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
#include "nodemetadata.h"
#include "porting.h"
#include "util/numeric.h"
#include "util/string.h"
#include "util/struct_serialize.h"
#include <sstream>
#include <string.h>
#if !defined(_WIN32)
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#if defined(__ANDROID__)
#include "porting_android.h"
#include <sys/syscall.h>
#else
#include <spawn.h>
#endif
#endif // !defined(_WIN32)

CSMController::CSMController(Client *client):
	m_client(client)
{
}

CSMController::~CSMController()
{
	stop();
}

bool CSMController::start()
{
	if (isStarted())
		return true;

	std::string client_path = porting::path_user + DIR_DELIM "client";

#if defined(_WIN32)
	char exe_path[MAX_PATH];

	PROCESS_INFORMATION process_info;

	STARTUPINFOA startup_info = {
		sizeof(startup_info), // cb
		nullptr, // lpReserved
		nullptr, // lpDesktop
		nullptr, // lpTitle
		0, // dwX
		0, // dwY
		0, // dwXSize
		0, // dwYSize
		0, // dwXCountChars
		0, // dwYCountChars
		0, // dwFillAttribute
		STARTF_USESTDHANDLES, // dwFlags
		0, // wShowWindow
		0, // cbReserved2
		nullptr, // lpReserved2
		GetStdHandle(STD_INPUT_HANDLE), // hStdInput
		GetStdHandle(STD_OUTPUT_HANDLE), // hStdOutput
		GetStdHandle(STD_ERROR_HANDLE), // hStdError
	};

	char env[] = {'\0'};

	char cmd_line[2048];

	const char *env_tz = getenv("TZ");

	SECURITY_ATTRIBUTES inherit_attr = { sizeof(inherit_attr), nullptr, true };

	m_ipc_shm = CreateFileMappingA(INVALID_HANDLE_VALUE, &inherit_attr, PAGE_READWRITE, 0,
			sizeof(IPCChannelShared), nullptr);
	if (m_ipc_shm == INVALID_HANDLE_VALUE)
		goto error_shm;

	m_ipc_sem_a = CreateSemaphoreA(&inherit_attr, 0, 1, nullptr);
	if (m_ipc_sem_a == INVALID_HANDLE_VALUE)
		goto error_sem_a;

	m_ipc_sem_b = CreateSemaphoreA(&inherit_attr, 0, 1, nullptr);
	if (m_ipc_sem_b == INVALID_HANDLE_VALUE)
		goto error_sem_b;

	m_ipc_shared = (IPCChannelShared *)MapViewOfFile(m_ipc_shm, FILE_MAP_ALL_ACCESS, 0, 0,
			sizeof(IPCChannelShared));
	if (!m_ipc_shared)
		goto error_map_shm;

	try {
		new (m_ipc_shared) IPCChannelShared;
		m_ipc = IPCChannelEnd::makeA(m_ipc_shared, m_ipc_sem_a, m_ipc_sem_b);
	} catch (...) {
		goto error_make_ipc;
	}

	porting::mt_snprintf(cmd_line, sizeof(cmd_line),
			"minetest --csm \"%s\" \"%s\" %llu %llu %llu",
			client_path.c_str(), env_tz ? env_tz : "", (unsigned long long)m_ipc_shm,
			(unsigned long long)m_ipc_sem_a, (unsigned long long)m_ipc_sem_b);

	if (!porting::getCurrentExecPath(exe_path, sizeof(exe_path)))
		goto error_exe_path;

	if (!CreateProcessA(exe_path, cmd_line, nullptr, nullptr, true, 0, env, nullptr,
			&startup_info, &process_info))
		goto error_create_process;

	CloseHandle(process_info.hThread);
	m_script_handle = process_info.hProcess;
	return true;

error_create_process:
error_exe_path:
	m_ipc_shared->~IPCChannelShared();
error_make_ipc:
	UnmapViewOfFile(m_ipc_shared);
error_map_shm:
	CloseHandle(m_ipc_sem_b);
error_sem_b:
	CloseHandle(m_ipc_sem_a);
error_sem_a:
	CloseHandle(m_ipc_shm);
error_shm:
#else
#if !defined(__ANDROID__)
	char exe_path[PATH_MAX];
#endif

	std::string shm_str;

	const char *env_tz = getenv("TZ");
	const char *env_tzdir = getenv("TZDIR");
	const char *argv[] = {
		"minetest", "--csm", client_path.c_str(), env_tz ? env_tz : "",
		env_tzdir ? env_tzdir : "", nullptr, nullptr
	};
#if !defined(__ANDROID__)
	char *const envp[] = {nullptr};
#endif

	int shm = -1;

#if defined(__ANDROID__)
	shm = syscall(SYS_memfd_create, "csm_shm", 0);
#else
	for (int i = 0; i < 100; i++) { // 100 tries
		std::string shm_name = std::string("/minetest") + std::to_string(myrand());
		shm = shm_open(shm_name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
		if (shm != -1)
			shm_unlink(shm_name.c_str());
		if (shm != -1 || errno != EEXIST)
			break;
	}
#endif
	if (shm == -1)
		goto error_shm;
#if defined(__ANDROID__)
	shm_str = std::to_string(SELF_EXEC_SPAWN_FD);
#else
	shm_str = std::to_string(shm);
#endif
	argv[5] = shm_str.c_str();

	{
		int flags = fcntl(shm, F_GETFD);
		if (flags < 0 || fcntl(shm, F_SETFD, flags & ~FD_CLOEXEC) < 0)
			goto error_fcntl;
	}

	if (ftruncate(shm, sizeof(IPCChannelShared)) != 0)
		goto error_ftruncate;

	m_ipc_shared = (IPCChannelShared *)mmap(nullptr, sizeof(IPCChannelShared),
			PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (m_ipc_shared == MAP_FAILED)
		goto error_mmap;

	try {
		new (m_ipc_shared) IPCChannelShared;
		m_ipc = IPCChannelEnd::makeA(m_ipc_shared);
	} catch (...) {
		goto error_make_ipc;
	}

#if !defined(__ANDROID__)
	if (!porting::getCurrentExecPath(exe_path, sizeof(exe_path)))
		goto error_exe_path;
#endif

#if defined(__ANDROID__)
	m_script_pid = porting::selfExecSpawn(argv, shm);
	if (m_script_pid < 0)
		goto error_spawn;
#else
	if (posix_spawn(&m_script_pid, exe_path,
			nullptr, nullptr, (char *const *)argv, envp) != 0)
		goto error_spawn;
#endif

	close(shm);
	return true;

error_spawn:
	m_script_pid = 0;
#if !defined(__ANDROID__)
error_exe_path:
#endif
error_make_ipc:
	m_ipc_shared->~IPCChannelShared();
	munmap(m_ipc_shared, sizeof(IPCChannelShared));
error_mmap:
error_ftruncate:
error_fcntl:
	close(shm);
error_shm:
#endif // !defined(_WIN32)
	errorstream << "Could not start CSM process" << std::endl;
	return false;
}

void CSMController::stop()
{
	if (!isStarted())
		return;

#if defined(_WIN32)
	TerminateProcess(m_script_handle, 0);
	CloseHandle(m_script_handle);
	m_script_handle = INVALID_HANDLE_VALUE;
	CloseHandle(m_ipc_shm);
	m_ipc_shared->~IPCChannelShared();
	UnmapViewOfFile(m_ipc_shared);
	CloseHandle(m_ipc_sem_a);
	CloseHandle(m_ipc_sem_b);
#else
#if defined(__ANDROID__)
	porting::selfExecKill(m_script_pid);
#else
	kill(m_script_pid, SIGKILL);
	waitpid(m_script_pid, nullptr, 0);
#endif
	m_script_pid = 0;
	m_ipc_shared->~IPCChannelShared();
	munmap(m_ipc_shared, sizeof(IPCChannelShared));
#endif // !defined(_WIN32)
}

void CSMController::runLoadMods()
{
	if (!isStarted())
		return;

	bool succeeded;
	{
		std::string defs;
		{
			std::ostringstream os(std::ios::binary);
			m_client->idef()->serialize(os, LATEST_PROTOCOL_VERSION);
			m_client->ndef()->serialize(os, LATEST_PROTOCOL_VERSION);
			defs = os.str();
		}
		succeeded = exchange(defs);
	}
	listen(succeeded);
}

void CSMController::runShutdown()
{
	if (!isStarted())
		return;

	listen(exchange(CSM_C2S_RUN_SHUTDOWN));
}

void CSMController::runClientReady()
{
	if (!isStarted())
		return;

	listen(exchange(CSM_C2S_RUN_CLIENT_READY));
}

void CSMController::runCameraReady()
{
	if (!isStarted())
		return;

	listen(exchange(CSM_C2S_RUN_CAMERA_READY));
}

void CSMController::runMinimapReady()
{
	if (!isStarted())
		return;

	listen(exchange(CSM_C2S_RUN_MINIMAP_READY));
}

bool CSMController::runSendingMessage(const std::string &message)
{
	if (!isStarted())
		return false;

	CSMC2SRunSendingMessage send(CSM_C2S_RUN_SENDING_MESSAGE, message);
	listen(exchange(send));
	return getDoneBool();
}

bool CSMController::runReceivingMessage(const std::string &message)
{
	if (!isStarted())
		return false;

	CSMC2SRunReceivingMessage send(CSM_C2S_RUN_RECEIVING_MESSAGE, message);
	listen(exchange(send));
	return getDoneBool();
}

bool CSMController::runDamageTaken(u16 damage)
{
	if (!isStarted())
		return false;

	CSMC2SRunDamageTaken send(CSM_C2S_RUN_DAMAGE_TAKEN, damage);
	listen(exchange(send));
	return getDoneBool();
}

bool CSMController::runHPModification(u16 hp)
{
	if (!isStarted())
		return false;

	CSMC2SRunHPModification send(CSM_C2S_RUN_HP_MODIFICATION, hp);
	listen(exchange(send));
	return getDoneBool();
}

void CSMController::runDeath()
{
	if (!isStarted())
		return;

	listen(exchange(CSM_C2S_RUN_DEATH));
}

void CSMController::runModchannelMessage(const std::string &channel, const std::string &sender,
		const std::string &message)
{
	if (!isStarted())
		return;

	CSMC2SRunModchannelMessage send(CSM_C2S_RUN_MODCHANNEL_MESSAGE, channel, sender, message);
	listen(exchange(send));
}

void CSMController::runModchannelSignal(const std::string &channel, ModChannelSignal signal)
{
	if (!isStarted())
		return;

	CSMC2SRunModchannelSignal send(CSM_C2S_RUN_MODCHANNEL_SIGNAL, channel, signal);
	listen(exchange(send));
}

bool CSMController::runFormspecInput(const std::string &formname, const StringMap &fields)
{
	if (!isStarted())
		return false;

	CSMC2SRunFormspecInput send(CSM_C2S_RUN_FORMSPEC_INPUT, formname, fields);
	listen(exchange(send));
	return getDoneBool();
}

bool CSMController::runInventoryOpen(const Inventory *inventory)
{
	if (!isStarted())
		return false;

	std::ostringstream os(std::ios::binary);
	inventory->serialize(os);
	CSMC2SRunInventoryOpen send(CSM_C2S_RUN_INVENTORY_OPEN, os.str());
	listen(exchange(send));
	return getDoneBool();
}

bool CSMController::runItemUse(const ItemStack &selected_item, const PointedThing &pointed)
{
	if (!isStarted())
		return false;

	CSMC2SRunItemUse send(CSM_C2S_RUN_ITEM_USE, selected_item.getItemString(), pointed);
	listen(exchange(send));
	return getDoneBool();
}

void CSMController::runPlacenode(const PointedThing &pointed, const ItemDefinition &def)
{
	if (!isStarted())
		return;

	CSMC2SRunPlacenode send(CSM_C2S_RUN_PLACENODE, pointed, def.name);
	listen(exchange(send));
}

bool CSMController::runPunchnode(v3s16 pos, MapNode n)
{
	if (!isStarted())
		return false;

	CSMC2SRunPunchnode send(CSM_C2S_RUN_PUNCHNODE, pos, n);
	listen(exchange(send));
	return getDoneBool();
}

bool CSMController::runDignode(v3s16 pos, MapNode n)
{
	if (!isStarted())
		return false;

	CSMC2SRunDignode send(CSM_C2S_RUN_DIGNODE, pos, n);
	listen(exchange(send));
	return getDoneBool();
}

void CSMController::runStep(float dtime)
{
	if (!isStarted())
		return;

	CSMC2SRunStep send(CSM_C2S_RUN_STEP, dtime);
	listen(exchange(send));
}

template<typename T>
bool CSMController::exchange(const T &send) noexcept
{
	bool succeeded = true;
	try {
		m_send_buf.clear();
		struct_serialize(m_send_buf, send);
		succeeded = m_ipc.exchange(m_send_buf.data(), m_send_buf.size(), m_timeout);
	} catch (...) {
		succeeded = false;
	}
	return succeeded;
}

template<typename T>
bool CSMController::deserializeMsg(T &recv) noexcept
{
	try {
		recv = struct_deserialize<T>(m_ipc.getRecvData(), m_ipc.getRecvSize());
	} catch (...) {
		return false;
	}
	return true;
}

void CSMController::listen(bool succeeded)
{
	u64 time_limit = porting::getTimeMs() + m_timeout;
	while (succeeded) {
		size_t size = m_ipc.getRecvSize();
		const void *data = m_ipc.getRecvData();
		CSMS2CMsgType type = CSM_S2C_INVALID;
		if (size >= sizeof(type))
			memcpy(&type, data, sizeof(type));
		switch (type) {
		case CSM_S2C_LOG:
			{
				csm_msg_owned_t<CSMS2CLog> recv;
				if ((succeeded = deserializeMsg(recv)
						&& std::get<1>(recv) >= 0 && std::get<1>(recv) < LL_MAX)) {
					if (g_logger.hasOutput(std::get<1>(recv))) {
						std::ostringstream os(std::ios::binary);
						safe_print_string(os, std::get<2>(recv));
						g_logger.logRaw(std::get<1>(recv), os.str());
					}
					succeeded = exchange(std::make_tuple<>());
				}
			}
			break;
		case CSM_S2C_GET_NODE:
			{
				csm_msg_owned_t<CSMS2CGetNode> recv;
				if ((succeeded = deserializeMsg(recv))) {
					bool pos_ok;
					MapNode n = m_client->getEnv().getMap().getNode(std::get<1>(recv), &pos_ok);
					CSMC2SGetNode send(n, pos_ok);
					succeeded = exchange(send);
				}
			}
			break;
		case CSM_S2C_ADD_NODE:
			{
				csm_msg_owned_t<CSMS2CAddNode> recv;
				if ((succeeded = deserializeMsg(recv))) {
					m_client->addNode(std::get<1>(recv), std::get<2>(recv), std::get<3>(recv));
					succeeded = exchange(std::make_tuple<>());
				}
			}
			break;
		case CSM_S2C_NODE_META_CLEAR:
			{
				csm_msg_owned_t<CSMS2CNodeMetaClear> recv;
				if ((succeeded = deserializeMsg(recv))) {
					m_client->getEnv().getMap().removeNodeMetadata(recv.second);
					succeeded = exchange(std::make_tuple<>());
				}
			}
			break;
		case CSM_S2C_NODE_META_CONTAINS:
			{
				csm_msg_owned_t<CSMS2CNodeMetaContains> recv;
				if ((succeeded = deserializeMsg(recv))) {
					NodeMetadata *meta =
							m_client->getEnv().getMap().getNodeMetadata(std::get<1>(recv));
					bool contains = meta && meta->contains(std::get<2>(recv));
					succeeded = exchange(contains);
				}
			}
			break;
		case CSM_S2C_NODE_META_SET_STRING:
			{
				csm_msg_owned_t<CSMS2CNodeMetaSetString> recv;
				if ((succeeded = deserializeMsg(recv))) {
					Map &map = m_client->getEnv().getMap();
					NodeMetadata *meta = map.getNodeMetadata(std::get<1>(recv));
					if (!meta && !std::get<3>(recv).empty()) {
						meta = new NodeMetadata(m_client->idef());
						if (!map.setNodeMetadata(std::get<1>(recv), meta)) {
							delete meta;
							meta = nullptr;
						}
					}
					bool modified = meta
							&& meta->setString(std::get<2>(recv), std::get<3>(recv));
					succeeded = exchange(modified);
				}
			}
			break;
		case CSM_S2C_NODE_META_GET_STRINGS:
			{
				csm_msg_owned_t<CSMS2CNodeMetaGetStrings> recv;
				if ((succeeded = deserializeMsg(recv))) {
					NodeMetadata *meta =
							m_client->getEnv().getMap().getNodeMetadata(std::get<1>(recv));
					if (meta) {
						succeeded = exchange(meta->getStrings());
					} else {
						StringMap strings;
						succeeded = exchange(strings);
					}
				}
			}
			break;
		case CSM_S2C_NODE_META_GET_KEYS:
			{
				csm_msg_owned_t<CSMS2CNodeMetaGetKeys> recv;
				if ((succeeded = deserializeMsg(recv))) {
					NodeMetadata *meta =
							m_client->getEnv().getMap().getNodeMetadata(std::get<1>(recv));
					std::ostringstream os(std::ios::binary);
					if (meta) {
						std::vector<std::string> keys_;
						const std::vector<std::string> &keys = meta->getKeys(&keys_);
						succeeded = exchange(keys);
					} else {
						std::vector<std::string> keys;
						succeeded = exchange(keys);
					}
				}
			}
			break;
		case CSM_S2C_NODE_META_GET_STRING:
			{
				csm_msg_owned_t<CSMS2CNodeMetaGetString> recv;
				if ((succeeded = deserializeMsg(recv))) {
					NodeMetadata *meta =
							m_client->getEnv().getMap().getNodeMetadata(std::get<1>(recv));
					if (meta) {
						const std::string &var = meta->getString(std::get<2>(recv), 2);
						succeeded = exchange(var);
					} else {
						succeeded = exchange(std::string());
					}
				}
			}
			break;
		case CSM_S2C_DONE:
			goto end;
		default:
			succeeded = false;
			break;
		}
		succeeded = succeeded && porting::getTimeMs() < time_limit;
	}
	stop();
	errorstream << "Error executing CSM" << std::endl;
end:
	m_send_buf.clear();
	m_send_buf.shrink_to_fit();
}

bool CSMController::getDoneBool()
{
	if (!isStarted())
		return false;
	csm_msg_owned_t<CSMS2CDoneBool> recv;
	return deserializeMsg(recv) && recv.second != 0;
}

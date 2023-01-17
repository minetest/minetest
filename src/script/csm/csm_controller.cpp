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
#include "util/serialize.h"
#include <sstream>
#include <string.h>
#if !defined(_WIN32)
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <spawn.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
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

	char exe_path[PATH_MAX];

	std::string client_path = porting::path_user + DIR_DELIM "client";

#if defined(_WIN32)
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

	char cmd_line[1024];

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

	porting::mt_snprintf(cmd_line, sizeof(cmd_line), "minetest --csm \"%s\" %llu %llu %llu",
			client_path.c_str(), (unsigned long long)m_ipc_shm,
			(unsigned long long)m_ipc_sem_a, (unsigned long long)m_ipc_sem_b);

	if (!porting::getCurrentExecPath(exe_path, sizeof(exe_path)))
		goto error_exe_path;

	if (!CreateProcessA(exe_path, cmd_line, nullptr, nullptr, true, 0, nullptr, nullptr,
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
	std::string shm_name;

	const char *argv[] = {"minetest", "--csm", client_path.c_str(), nullptr, nullptr};

	int shm = -1;
	for (int i = 0; i < 100; i++) { // 100 tries
		shm_name = std::string("/minetest") + std::to_string(myrand());
		shm = shm_open(shm_name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
		if (shm != -1 || errno != EEXIST)
			break;
	}
	if (shm == -1)
		goto error_shm;
	argv[3] = shm_name.c_str();

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

	close(shm);
	shm = -1;

	if (!porting::getCurrentExecPath(exe_path, sizeof(exe_path)))
		goto error_exe_path;

	if (posix_spawn(&m_script_pid, exe_path,
			nullptr, nullptr, (char *const *)argv, nullptr) != 0)
		goto error_spawn;

	return true;

error_spawn:
	m_script_pid = 0;
error_exe_path:
error_make_ipc:
	m_ipc_shared->~IPCChannelShared();
	munmap(m_ipc_shared, sizeof(IPCChannelShared));
error_mmap:
error_ftruncate:
	shm_unlink(shm_name.c_str());
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
	kill(m_script_pid, SIGKILL);
	waitpid(m_script_pid, nullptr, 0);
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
		succeeded = m_ipc.exchange(defs.size(), defs.data(), m_timeout);
	}
	listen(succeeded);
}

void CSMController::runShutdown()
{
	if (!isStarted())
		return;

	listen(m_ipc.exchange(CSM_C2S_RUN_SHUTDOWN, m_timeout));
}

void CSMController::runClientReady()
{
	if (!isStarted())
		return;

	listen(m_ipc.exchange(CSM_C2S_RUN_CLIENT_READY, m_timeout));
}

void CSMController::runCameraReady()
{
	if (!isStarted())
		return;

	listen(m_ipc.exchange(CSM_C2S_RUN_CAMERA_READY, m_timeout));
}

void CSMController::runMinimapReady()
{
	if (!isStarted())
		return;

	listen(m_ipc.exchange(CSM_C2S_RUN_MINIMAP_READY, m_timeout));
}

bool CSMController::runSendingMessage(const std::string &message)
{
	if (!isStarted())
		return false;

	std::vector<u8> send;
	CSMC2SMsgType type = CSM_C2S_RUN_SENDING_MESSAGE;
	send.resize(sizeof(type) + message.size());
	memcpy(send.data(), &type, sizeof(type));
	memcpy(send.data() + sizeof(type), message.data(), message.size());
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
	return getDoneBool();
}

bool CSMController::runReceivingMessage(const std::string &message)
{
	if (!isStarted())
		return false;

	std::vector<u8> send;
	CSMC2SMsgType type = CSM_C2S_RUN_RECEIVING_MESSAGE;
	send.resize(sizeof(type) + message.size());
	memcpy(send.data(), &type, sizeof(type));
	memcpy(send.data() + sizeof(type), message.data(), message.size());
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
	return getDoneBool();
}

bool CSMController::runDamageTaken(u16 damage)
{
	if (!isStarted())
		return false;

	CSMC2SRunDamageTaken send;
	send.damage = damage;
	listen(m_ipc.exchange(send, m_timeout));
	return getDoneBool();
}

bool CSMController::runHPModification(u16 hp)
{
	if (!isStarted())
		return false;

	CSMC2SRunHPModification send;
	send.hp = hp;
	listen(m_ipc.exchange(send, m_timeout));
	return getDoneBool();
}

void CSMController::runDeath()
{
	if (!isStarted())
		return;

	listen(m_ipc.exchange(CSM_C2S_RUN_DEATH, m_timeout));
}

void CSMController::runModchannelMessage(const std::string &channel, const std::string &sender,
		const std::string &message)
{
	if (!isStarted())
		return;

	std::vector<u8> send;
	CSMC2SRunModchannelMessage header;
	header.channel_size = channel.size();
	header.sender_size = sender.size();
	header.message_size = message.size();
	send.resize(sizeof(header) + channel.size() + sender.size() + message.size());
	memcpy(send.data(), &header, sizeof(header));
	memcpy(send.data() + sizeof(header), channel.data(), channel.size());
	memcpy(send.data() + sizeof(header) + channel.size(), sender.data(), sender.size());
	memcpy(send.data() + sizeof(header) + channel.size() + sender.size(),
			message.data(), message.size());
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
}

void CSMController::runModchannelSignal(const std::string &channel, ModChannelSignal signal)
{
	if (!isStarted())
		return;

	std::vector<u8> send;
	CSMC2SRunModchannelSignal header;
	header.signal = signal;
	send.resize(sizeof(header) + channel.size());
	memcpy(send.data(), &header, sizeof(header));
	memcpy(send.data() + sizeof(header), channel.data(), channel.size());
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
}

bool CSMController::runFormspecInput(const std::string &formname, const StringMap &fields)
{
	if (!isStarted())
		return false;

	std::ostringstream os(std::ios::binary);
	CSMC2SMsgType type = CSM_C2S_RUN_FORMSPEC_INPUT;
	os.write((char *)&type, sizeof(type));
	os << serializeString16(formname);
	writeU32(os, fields.size());
	for (const auto &pair : fields) {
		os << serializeString16(pair.first);
		os << serializeString16(pair.second);
	}
	std::string send = os.str();
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
	return getDoneBool();
}

bool CSMController::runInventoryOpen(const Inventory *inventory)
{
	if (!isStarted())
		return false;

	std::ostringstream os(std::ios::binary);
	CSMC2SMsgType type = CSM_C2S_RUN_INVENTORY_OPEN;
	os.write((char *)&type, sizeof(type));
	inventory->serialize(os);
	std::string send = os.str();
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
	return getDoneBool();
}

bool CSMController::runItemUse(const ItemStack &selected_item, const PointedThing &pointed)
{
	if (!isStarted())
		return false;

	CSMC2SRunItemUse header;
	header.pointed_thing = pointed;
	std::string itemstring = selected_item.getItemString();
	std::vector<u8> send;
	send.resize(sizeof(header) + itemstring.size());
	memcpy(send.data(), &header, sizeof(header));
	memcpy(send.data() + sizeof(header), itemstring.data(), itemstring.size());
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
	return getDoneBool();
}

void CSMController::runPlacenode(const PointedThing &pointed, const ItemDefinition &def)
{
	if (!isStarted())
		return;

	CSMC2SRunPlacenode header;
	header.pointed_thing = pointed;
	std::vector<u8> send;
	send.resize(sizeof(header) + def.name.size());
	memcpy(send.data(), &header, sizeof(header));
	memcpy(send.data() + sizeof(header), def.name.data(), def.name.size());
	listen(m_ipc.exchange(send.size(), send.data(), m_timeout));
}

bool CSMController::runPunchnode(v3s16 pos, MapNode n)
{
	if (!isStarted())
		return false;

	CSMC2SRunPunchnode send;
	send.pos = pos;
	send.n = n;
	listen(m_ipc.exchange(send, m_timeout));
	return getDoneBool();
}

bool CSMController::runDignode(v3s16 pos, MapNode n)
{
	if (!isStarted())
		return false;

	CSMC2SRunDignode send;
	send.pos = pos;
	send.n = n;
	listen(m_ipc.exchange(send, m_timeout));
	return getDoneBool();
}

void CSMController::runStep(float dtime)
{
	if (!isStarted())
		return;

	CSMC2SRunStep msg;
	msg.dtime = dtime;
	listen(m_ipc.exchange(msg, m_timeout));
}

void CSMController::listen(bool succeeded)
{
	while (succeeded) {
		size_t size = m_ipc.getRecvSize();
		const void *data = m_ipc.getRecvData();
		CSMS2CMsgType type = CSM_S2C_INVALID;
		if (size >= sizeof(type))
			memcpy(&type, data, sizeof(type));
		switch (type) {
		case CSM_S2C_LOG:
			if ((succeeded = size >= sizeof(CSMS2CLog))) {
				CSMS2CLog recv;
				memcpy(&recv, data, sizeof(recv));
				if ((succeeded = recv.level >= 0 && recv.level < LL_MAX)) {
					std::string line((char *)data + sizeof(recv), size - sizeof(recv));
					g_logger.logRaw(recv.level, line);
					int dummy;
					succeeded = m_ipc.exchange(0, &dummy, m_timeout);
				}
			}
			break;
		case CSM_S2C_GET_NODE:
			if ((succeeded = size >= sizeof(CSMS2CGetNode))) {
				CSMS2CGetNode recv;
				memcpy(&recv, data, sizeof(recv));
				CSMC2SGetNode send;
				send.n = m_client->getEnv().getMap().getNode(recv.pos, &send.pos_ok);
				succeeded = m_ipc.exchange(send, m_timeout);
			}
			break;
		case CSM_S2C_ADD_NODE:
			if ((succeeded = size >= sizeof(CSMS2CAddNode))) {
				CSMS2CAddNode recv;
				memcpy(&recv, data, sizeof(recv));
				m_client->addNode(recv.pos, recv.n, recv.remove_metadata);
				int dummy;
				succeeded = m_ipc.exchange(0, &dummy, m_timeout);
			}
			break;
		case CSM_S2C_NODE_META_CLEAR:
			if ((succeeded = size >= sizeof(CSMS2CNodeMetaClear))) {
				CSMS2CNodeMetaClear recv;
				memcpy(&recv, data, sizeof(recv));
				m_client->getEnv().getMap().removeNodeMetadata(recv.pos);
				int dummy;
				succeeded = m_ipc.exchange(0, &dummy, m_timeout);
			}
			break;
		case CSM_S2C_NODE_META_CONTAINS:
			if ((succeeded = size >= sizeof(CSMS2CNodeMetaContains))) {
				CSMS2CNodeMetaContains recv_header;
				memcpy(&recv_header, data, sizeof(recv_header));
				std::string name((char *)data + sizeof(recv_header),
						size - sizeof(recv_header));
				NodeMetadata *meta =
						m_client->getEnv().getMap().getNodeMetadata(recv_header.pos);
				bool contains = meta && meta->contains(name);
				succeeded = m_ipc.exchange(contains, m_timeout);
			}
			break;
		case CSM_S2C_NODE_META_SET_STRING:
			if ((succeeded = size >= sizeof(CSMS2CNodeMetaSetString))) {
				std::istringstream is(std::string((char *)data, size), std::ios::binary);
				CSMS2CNodeMetaSetString recv_header;
				is.read((char *)&recv_header, sizeof(recv_header));
				std::string name = deSerializeString16(is);
				std::string var = deSerializeString32(is);
				Map &map = m_client->getEnv().getMap();
				NodeMetadata *meta = map.getNodeMetadata(recv_header.pos);
				if (!meta && !var.empty()) {
					meta = new NodeMetadata(m_client->idef());
					if (!map.setNodeMetadata(recv_header.pos, meta)) {
						delete meta;
						meta = nullptr;
					}
				}
				bool modified = meta && meta->setString(name, var);
				succeeded = m_ipc.exchange(modified, m_timeout);
			}
			break;
		case CSM_S2C_NODE_META_GET_STRINGS:
			if ((succeeded = size >= sizeof(CSMS2CNodeMetaGetStrings))) {
				CSMS2CNodeMetaGetStrings recv;
				memcpy(&recv, data, sizeof(recv));
				NodeMetadata *meta = m_client->getEnv().getMap().getNodeMetadata(recv.pos);
				std::ostringstream os(std::ios::binary);
				if (meta) {
					const StringMap &strings = meta->getStrings();
					writeU32(os, strings.size());
					for (const auto &pair : strings)
						os << serializeString16(pair.first) << serializeString32(pair.second);
				} else {
					writeU32(os, 0);
				}
				std::string send = os.str();
				succeeded = m_ipc.exchange(send.size(), send.data(), m_timeout);
			}
			break;
		case CSM_S2C_NODE_META_GET_KEYS:
			if ((succeeded = size >= sizeof(CSMS2CNodeMetaGetKeys))) {
				CSMS2CNodeMetaGetKeys recv;
				memcpy(&recv, data, sizeof(recv));
				NodeMetadata *meta = m_client->getEnv().getMap().getNodeMetadata(recv.pos);
				std::ostringstream os(std::ios::binary);
				if (meta) {
					const StringMap &strings = meta->getStrings();
					writeU32(os, strings.size());
					for (const auto &pair : strings)
						os << serializeString16(pair.first);
				} else {
					writeU32(os, 0);
				}
				std::string send = os.str();
				succeeded = m_ipc.exchange(send.size(), send.data(), m_timeout);
			}
			break;
		case CSM_S2C_NODE_META_GET_STRING:
			if ((succeeded = size >= sizeof(CSMS2CNodeMetaGetString))) {
				CSMS2CNodeMetaGetString recv_header;
				memcpy(&recv_header, data, sizeof(recv_header));
				std::string name((char *)data + sizeof(recv_header),
						size - sizeof(recv_header));
				NodeMetadata *meta =
						m_client->getEnv().getMap().getNodeMetadata(recv_header.pos);
				if (meta) {
					const std::string &send = meta->getString(name, 2);
					succeeded = m_ipc.exchange(send.size(), send.data(), m_timeout);
				} else {
					int dummy;
					succeeded = m_ipc.exchange(0, &dummy, m_timeout);
				}
			}
			break;
		case CSM_S2C_DONE:
			return;
		default:
			succeeded = false;
			break;
		}
	}
	stop();
	errorstream << "Error executing CSM" << std::endl;
}

bool CSMController::getDoneBool()
{
	CSMS2CDoneBool recv;
	recv.value = 0;
	if (isStarted() && m_ipc.getRecvSize() >= sizeof(recv))
		memcpy(&recv, m_ipc.getRecvData(), sizeof(recv));
	return recv.value != 0;
}

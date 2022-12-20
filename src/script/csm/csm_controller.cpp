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
#include "csm_script_process.h"
#include "csm_message.h"
#include "client/client.h"
#include "log.h"
#include "map.h"
#include "network/networkprotocol.h"
#include "nodedef.h"
#include "util/numeric.h"
#include "util/string.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

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

	std::string shm_name;

	const char *argv[] = {"minetest", "--csm", nullptr, nullptr};

	int shm = -1;
	for (int i = 0; i < 100; i++) { // 100 tries
		shm_name = std::string("/minetest") + std::to_string(myrand());
		shm = shm_open(shm_name.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
		if (shm != -1 || errno != EEXIST)
			break;
	}
	if (shm == -1)
		goto error_shm;
	argv[2] = shm_name.c_str();

	if (ftruncate(shm, sizeof(IPCChannelShared)) != 0)
		goto error_ftruncate;

	m_ipc_shared = (IPCChannelShared *)mmap(nullptr, sizeof(IPCChannelShared),
			PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if (m_ipc_shared == MAP_FAILED)
		goto error_mmap;

	posix_spawn_file_actions_t file_actions;
	if (posix_spawn_file_actions_init(&file_actions) != 0)
		goto error_file_actions_init;

	if (posix_spawn_file_actions_addclose(&file_actions, shm) != 0)
		goto error_file_actions;

	if (posix_spawn(&m_script_pid, "/proc/self/exe",
			&file_actions, nullptr, (char *const *)argv, nullptr) != 0)
		goto error_spawn;

	posix_spawn_file_actions_destroy(&file_actions);
	close(shm);

	new (m_ipc_shared) IPCChannelShared;
	m_ipc = IPCChannelEnd::makeA(m_ipc_shared);
	return true;

error_spawn:
	m_script_pid = 0;
error_file_actions:
	posix_spawn_file_actions_destroy(&file_actions);
error_file_actions_init:
	munmap(m_ipc_shared, sizeof(IPCChannelShared));
error_mmap:
error_ftruncate:
	shm_unlink(shm_name.c_str());
	close(shm);
error_shm:
	errorstream << "Could not start CSM process" << std::endl;
	return false;
}

void CSMController::stop()
{
	if (!isStarted())
		return;

	kill(m_script_pid, SIGKILL);
	m_script_pid = 0;
	munmap(m_ipc_shared, sizeof(IPCChannelShared));
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
		CSMMsgType type = CSM_INVALID_MSG_TYPE;
		if (size >= sizeof(type))
			memcpy(&type, data, sizeof(type));
		switch (type) {
		case CSM_S2C_GET_NODE:
			if (size >= sizeof(CSMS2CGetNode)) {
				CSMS2CGetNode msg;
				memcpy(&msg, data, sizeof(msg));
				MapNode n = m_client->getEnv().getMap().getNode(msg.pos);
				succeeded = m_ipc.exchange(n, m_timeout);
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

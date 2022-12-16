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
#include <signal.h>
#include <spawn.h>
#include <string.h>

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

	const char *argv[] = {"minetest", "--csm", nullptr};

	int controller2script[2] = {-1, -1};
	int script2controller[2] = {-1, -1};
	if (pipe(controller2script) < 0 || pipe(script2controller) < 0)
		goto error_pipe;

	m_from_script = fdopen(script2controller[0], "rb");
	if (!m_from_script)
		goto error_fdopen_from_script;

	m_to_script = fdopen(controller2script[1], "wb");
	if (!m_to_script)
		goto error_fdopen_to_script;

	posix_spawn_file_actions_t file_actions;
	if (posix_spawn_file_actions_init(&file_actions) != 0)
		goto error_file_actions_init;

	if (controller2script[0] != CSM_SCRIPT_READ_FD) {
		if (posix_spawn_file_actions_adddup2(&file_actions,
				controller2script[0], CSM_SCRIPT_READ_FD) != 0)
			goto error_file_actions;
		if (posix_spawn_file_actions_addclose(&file_actions, controller2script[0]) != 0)
			goto error_file_actions;
	}
	if (script2controller[1] != CSM_SCRIPT_WRITE_FD) {
		if (posix_spawn_file_actions_adddup2(&file_actions,
				script2controller[1], CSM_SCRIPT_WRITE_FD) != 0)
			goto error_file_actions;
		if (posix_spawn_file_actions_addclose(&file_actions, script2controller[1]) != 0)
			goto error_file_actions;
	}
	if (posix_spawn_file_actions_addclose(&file_actions, script2controller[0]) != 0)
		goto error_file_actions;
	if (posix_spawn_file_actions_addclose(&file_actions, controller2script[1]) != 0)
		goto error_file_actions;

	if (posix_spawn(&m_script_pid, "/proc/self/exe",
			&file_actions, nullptr, (char *const *)argv, nullptr) != 0)
		goto error_spawn;

	posix_spawn_file_actions_destroy(&file_actions);
	close(script2controller[1]);
	close(controller2script[0]);

	return true;

error_spawn:
	m_script_pid = 0;
error_file_actions:
	posix_spawn_file_actions_destroy(&file_actions);
error_file_actions_init:
	fclose(m_to_script);
	controller2script[1] = -1;
error_fdopen_to_script:
	fclose(m_from_script);
	script2controller[0] = -1;
error_fdopen_from_script:
error_pipe:
	close(controller2script[0]);
	close(controller2script[1]);
	close(script2controller[0]);
	close(script2controller[1]);
	errorstream << "Could not start CSM process" << std::endl;
	return false;
}

void CSMController::stop()
{
	if (!isStarted())
		return;

	kill(m_script_pid, SIGKILL);
	m_script_pid = 0;
	fclose(m_to_script);
	m_to_script = nullptr;
	fclose(m_from_script);
	m_from_script = nullptr;
}

void CSMController::runLoadMods()
{
	if (!isStarted())
		return;

	{
		std::string nodedef;
		{
			std::ostringstream os;
			m_client->ndef()->serialize(os, LATEST_PROTOCOL_VERSION);
			nodedef = os.str();
		}
		csm_send_msg(m_to_script, CSMMsgType::C2S_RUN_LOAD_MODS,
				nodedef.size(), nodedef.data());
	}
	listen();
}

void CSMController::runStep(float dtime)
{
	if (!isStarted())
		return;

	csm_send_msg(m_to_script, CSMMsgType::C2S_RUN_STEP, sizeof(dtime), &dtime);
	listen();
}

void CSMController::listen()
{
	Optional<CSMRecvMsg> msg;
	while ((msg = csm_recv_msg(m_from_script))) {
		switch (msg->type) {
		case CSMMsgType::S2C_GET_NODE:
			if (msg->data.size() >= sizeof(v3s16)) {
				v3s16 pos;
				memcpy(&pos, msg->data.data(), sizeof(v3s16));
				MapNode n = m_client->getEnv().getMap().getNode(pos);
				if (csm_send_msg(m_to_script, CSMMsgType::C2S_GET_NODE, sizeof(n), &n))
					break;
			}
			goto error;
		case CSMMsgType::S2C_DONE:
			return;
		default:
			goto error;
		}
	}
error:
	stop();
	errorstream << "Error executing CSM" << std::endl;
}

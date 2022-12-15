#include "sscsm_controller.h"
#include "sscsm_script_process.h"
#include "sscsm_message.h"
#include "client/client.h"
#include "log.h"
#include "map.h"
#include <signal.h>
#include <spawn.h>
#include <string.h>

SSCSMController::SSCSMController(Client *client):
	m_client(client)
{
}

SSCSMController::~SSCSMController()
{
	stop();
}

bool SSCSMController::start()
{
	if (isStarted())
		return true;

	const char *argv[] = {"minetest", "--sscsm", nullptr};

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

	if (controller2script[0] != SSCSM_SCRIPT_READ_FD) {
		if (posix_spawn_file_actions_adddup2(&file_actions,
				controller2script[0], SSCSM_SCRIPT_READ_FD) != 0)
			goto error_file_actions;
		if (posix_spawn_file_actions_addclose(&file_actions, controller2script[0]) != 0)
			goto error_file_actions;
	}
	if (script2controller[1] != SSCSM_SCRIPT_WRITE_FD) {
		if (posix_spawn_file_actions_adddup2(&file_actions,
				script2controller[1], SSCSM_SCRIPT_WRITE_FD) != 0)
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
	errorstream << "Could not start SSCSM process" << std::endl;
	return false;
}

void SSCSMController::stop()
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

void SSCSMController::runStep(float dtime)
{
	if (!isStarted())
		return;

	sscsm_send_msg(m_to_script, SSCSMMsgType::C2S_RUN_STEP, sizeof(dtime), &dtime);
	listen();
}

void SSCSMController::runLoadMods()
{
	if (!isStarted())
		return;

	sscsm_send_msg(m_to_script, SSCSMMsgType::C2S_RUN_LOAD_MODS, 0, nullptr);
	listen();
}

void SSCSMController::listen()
{
	std::vector<u8> send; // Data to send, shared for efficiency
	Optional<SSCSMRecvMsg> msg;
	while ((msg = sscsm_recv_msg(m_from_script))) {
		switch (msg->type) {
		case SSCSMMsgType::S2C_GET_NODE:
			if (msg->data.size() >= sizeof(v3s16)) {
				v3s16 pos;
				memcpy(&pos, msg->data.data(), sizeof(v3s16));
				MapNode n = m_client->getEnv().getMap().getNode(pos);
				const std::string &name = m_client->ndef()->get(n).name;
				send.resize(name.size() + 2);
				send[0] = n.getParam1();
				send[1] = n.getParam2();
				memcpy(send.data() + 2, name.c_str(), name.size());
				if (sscsm_send_msg(m_to_script, SSCSMMsgType::C2S_GET_NODE,
						send.size(), send.data()))
					break;
			}
			goto error;
		case SSCSMMsgType::S2C_DONE:
			return;
		default:
			goto error;
		}
	}
error:
	stop();
	errorstream << "Error executing SSCSM" << std::endl;
}

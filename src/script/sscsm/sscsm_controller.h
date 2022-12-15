#pragma once

#include <stdio.h>
#include <unistd.h>

class Client;

class SSCSMController
{
public:
	SSCSMController(Client *client);
	~SSCSMController();

	bool start();
	void stop();

	bool isStarted() { return m_script_pid != 0; }

	void runStep(float dtime);
	void runLoadMods();

private:
	void listen();

	Client *const m_client;
	pid_t m_script_pid = 0;
	FILE *m_to_script = nullptr;
	FILE *m_from_script = nullptr;
};

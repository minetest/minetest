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

#pragma once

#include <unistd.h>
#include "irrlichttypes.h"
#include "threading/ipc_channel.h"
#include "util/basic_macros.h"

class Client;

class CSMController
{
public:
	CSMController(Client *client);
	~CSMController();

	DISABLE_CLASS_COPY(CSMController)

	bool start();
	void stop();

	bool isStarted() { return m_script_pid != 0; }

	void runLoadMods();
	void runShutdown();
	void runClientReady();
	void runCameraReady();
	void runMinimapReady();
	bool runSendingMessage(const std::string &message);
	bool runReceivingMessage(const std::string &message);
	bool runHPModification(u16 hp);
	void runModchannelMessage(const std::string &channel, const std::string &sender,
			const std::string &message);
	void runModchannelSignal(const std::string &channel, int signal);
	void runStep(float dtime);

private:
	void listen(bool succeeded);

	Client *const m_client;
	int m_timeout = 1000;
	pid_t m_script_pid = 0;
	IPCChannelShared *m_ipc_shared = nullptr;
	IPCChannelEnd m_ipc;
};

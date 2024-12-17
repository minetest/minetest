
#pragma once

#include <memory>
#include "client/client.h"
#include "threading/thread.h"
#include "sscsm_controller.h"
#include "sscsm_irequest.h"

// The thread that runs SSCSM code.
// Meant to be replaced by a sandboxed process.
struct SSCSMEnvironment : Thread
{
	std::shared_ptr<StupidChannel> m_channel;

	SSCSMEnvironment(std::shared_ptr<StupidChannel> channel) :
		Thread("SSCSMEnvironment-thread"),
		m_channel(std::move(channel))
	{
	}

	void *run() override;

	SerializedSSCSMAnswer exchange(SerializedSSCSMRequest req);

	void runEventOnStep();

	std::unique_ptr<ISSCSMEvent> cmdPollNextEvent();
	MapNode cmdGetNode(v3s16 pos);
};

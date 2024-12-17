
#pragma once

#include <memory>
#include "irrlichttypes.h"
#include "sscsm_irequest.h"

struct SSCSMEnvironment;
class StupidChannel;

struct SSCSMController
{
	std::unique_ptr<SSCSMEnvironment> m_thread;
	std::shared_ptr<StupidChannel> m_channel;

	static std::unique_ptr<SSCSMController> create();

	SSCSMController(std::unique_ptr<SSCSMEnvironment> thread,
			std::shared_ptr<StupidChannel> channel);

	~SSCSMController();

	SerializedSSCSMAnswer handleRequest(ISSCSMRequest *req, Client *client);

	// Handles requests until the next event is polled
	void runEvent(int event, Client *client);

	void eventTearDown(Client *client);
	void eventOnStep(f32 dtime, Client *client);
};

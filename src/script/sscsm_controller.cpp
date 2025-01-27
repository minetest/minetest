// SPDX-FileCopyrightText: 2024 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "sscsm_controller.h"
#include "sscsm_environment.h"
#include "sscsm_requests.h"
#include "sscsm_events.h"
#include "sscsm_stupid_channel.h"

std::unique_ptr<SSCSMController> SSCSMController::create()
{
	auto channel = std::make_shared<StupidChannel>();
	auto thread = std::make_unique<SSCSMEnvironment>(channel);
	thread->start();

	// Wait for thread to finish initializing.
	auto req0 = deserializeSSCSMRequest(channel->recvB());
	FATAL_ERROR_IF(!dynamic_cast<SSCSMRequestPollNextEvent *>(req0.get()),
			"First request must be pollEvent.");

	return std::make_unique<SSCSMController>(std::move(thread), channel);
}

SSCSMController::SSCSMController(std::unique_ptr<SSCSMEnvironment> thread,
		std::shared_ptr<StupidChannel> channel) :
	m_thread(std::move(thread)), m_channel(std::move(channel))
{
}

SSCSMController::~SSCSMController()
{
	// send tear-down
	m_channel->sendB(serializeSSCSMAnswer(SSCSMAnswerPollNextEvent{
			std::make_unique<SSCSMEventTearDown>()
		}));
	// wait for death
	m_thread->stop();
	m_thread->wait();
}

SerializedSSCSMAnswer SSCSMController::handleRequest(Client *client, ISSCSMRequest *req)
{
	return req->exec(client);
}

void SSCSMController::runEvent(Client *client, std::unique_ptr<ISSCSMEvent> event)
{
	auto answer = serializeSSCSMAnswer(SSCSMAnswerPollNextEvent{std::move(event)});

	while (true) {
		auto request = deserializeSSCSMRequest(m_channel->exchangeB(std::move(answer)));

		if (dynamic_cast<SSCSMRequestPollNextEvent *>(request.get()) != nullptr) {
			break;
		}

		answer = handleRequest(client, request.get());
	}
}

void SSCSMController::eventOnStep(Client *client, f32 dtime)
{
	runEvent(client, std::make_unique<SSCSMEventOnStep>(dtime));
}

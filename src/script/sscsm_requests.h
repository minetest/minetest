// SPDX-FileCopyrightText: 2024 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "sscsm_irequest.h"
#include "sscsm_ievent.h"
#include "mapnode.h"
#include "map.h"
#include "client/client.h"

struct SSCSMAnswerPollNextEvent : public ISSCSMAnswer
{
	std::unique_ptr<ISSCSMEvent> next_event;

	SSCSMAnswerPollNextEvent(std::unique_ptr<ISSCSMEvent> next_event_) :
		next_event(std::move(next_event_))
	{
	}
};

struct SSCSMRequestPollNextEvent : public ISSCSMRequest
{
	SerializedSSCSMAnswer exec(Client *client) override
	{
		FATAL_ERROR("SSCSMRequestPollNextEvent needs to be handled by SSCSMControler::runEvent()");
	}
};

struct SSCSMAnswerGetNode : public ISSCSMAnswer
{
	MapNode node;

	SSCSMAnswerGetNode(MapNode node_) : node(node_) {}
};

struct SSCSMRequestGetNode : public ISSCSMRequest
{
	v3s16 pos;

	SSCSMRequestGetNode(v3s16 pos_) : pos(pos_) {}

	SerializedSSCSMAnswer exec(Client *client) override
	{
		MapNode node = client->getEnv().getMap().getNode(pos);

		return serializeSSCSMAnswer(SSCSMAnswerGetNode{node});
	}
};

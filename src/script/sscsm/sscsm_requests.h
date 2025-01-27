// SPDX-FileCopyrightText: 2024 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "sscsm_irequest.h"
#include "sscsm_ievent.h"
#include "mapnode.h"
#include "map.h"
#include "client/client.h"

struct SSCSMRequestPollNextEvent : public ISSCSMRequest
{
	struct Answer : public ISSCSMAnswer
	{
		std::unique_ptr<ISSCSMEvent> next_event;

		Answer(std::unique_ptr<ISSCSMEvent> next_event_) :
			next_event(std::move(next_event_))
		{
		}
	};

	SerializedSSCSMAnswer exec(Client *client) override
	{
		FATAL_ERROR("SSCSMRequestPollNextEvent needs to be handled by SSCSMControler::runEvent()");
	}
};

struct SSCSMRequestGetNode : public ISSCSMRequest
{
	struct Answer : public ISSCSMAnswer
	{
		MapNode node;

		Answer(MapNode node_) : node(node_) {}
	};

	v3s16 pos;

	SSCSMRequestGetNode(v3s16 pos_) : pos(pos_) {}

	SerializedSSCSMAnswer exec(Client *client) override
	{
		MapNode node = client->getEnv().getMap().getNode(pos);

		return serializeSSCSMAnswer(Answer{node});
	}
};

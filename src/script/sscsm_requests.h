
#pragma once

#include "sscsm_irequest.h"
#include "mapnode.h"
#include "map.h"
#include "client/client.h"

struct SSCSMAnswerPollNextEvent : public ISSCSMAnswer
{
	int next_event;

	SSCSMAnswerPollNextEvent(int next_event_) : next_event(next_event_) {}
};

struct SSCSMRequestPollNextEvent : public ISSCSMRequest
{
	SerializedSSCSMAnswer exec(SSCSMController *cntrl, Client *client) override
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

	SerializedSSCSMAnswer exec(SSCSMController *cntrl, Client *client) override
	{
		MapNode node = client->getEnv().getMap().getNode(pos);

		return serializeSSCSMAnswer(SSCSMAnswerGetNode{node});
	}
};

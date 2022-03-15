#pragma once

#include "mapnode.h"
#include "util/string.h"

namespace api
{
namespace server
{
/*
 * Node callback events
 */

class Node
{
public:
	virtual bool node_on_punch(v3s16 p, MapNode node, ServerActiveObject *puncher,
			const PointedThing &pointed)
	{
		return false;
	}
	virtual bool node_on_dig(v3s16 p, MapNode node, ServerActiveObject *digger)
	{
		return false;
	}
	virtual void node_on_construct(v3s16 p, MapNode node) {}
	virtual void node_on_destruct(v3s16 p, MapNode node) {}
	virtual bool node_on_flood(v3s16 p, MapNode node, MapNode newnode)
	{
		return false;
	}
	virtual void node_after_destruct(v3s16 p, MapNode node) {}
	virtual bool node_on_timer(v3s16 p, MapNode node, f32 dtime) { return false; }
	virtual void node_on_receive_fields(v3s16 p, const std::string &formname,
			const StringMap &fields, ServerActiveObject *sender)
	{
	}
};
}
}
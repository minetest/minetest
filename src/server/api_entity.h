#pragma once

struct collisionMoveResult;
struct ObjectProperties;
struct ToolCapabilities;

namespace api
{
namespace server
{
/*
 * Entity callback events
 */

class Entity
{
public:
	virtual void addObjectReference(ServerActiveObject *obj) {}
	virtual void removeObjectReference(ServerActiveObject *obj) {}

	virtual bool luaentity_Add(u16 id, const char *name) { return false; }
	virtual void luaentity_Activate(
			u16 id, const std::string &staticdata, u32 dtime_s)
	{
	}
	virtual void luaentity_Remove(u16 id) {}
	virtual std::string luaentity_GetStaticdata(u16 id) { return ""; }
	virtual void luaentity_GetProperties(
			u16 id, ServerActiveObject *self, ObjectProperties *prop)
	{
	}
	virtual void on_entity_step(
			u16 id, float dtime, const collisionMoveResult *moveresult)
	{
	}
	virtual bool on_entity_punched(u16 id, ServerActiveObject *puncher,
			float time_from_last_punch, const ToolCapabilities *toolcap,
			v3f dir, s16 damage)
	{
		return false;
	}
	virtual bool on_entity_death(u16 id, ServerActiveObject *killer) { return false; }
	virtual void on_entity_rightclick(u16 id, ServerActiveObject *clicker) {}
	virtual void luaentity_on_attach_child(u16 id, ServerActiveObject *child) {}
	virtual void luaentity_on_detach_child(u16 id, ServerActiveObject *child) {}
	virtual void luaentity_on_detach(u16 id, ServerActiveObject *parent) {}
};
}
}
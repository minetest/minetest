#pragma once

namespace api
{
namespace server
{
/*
 * Environment callback events
 */
class Environment
{
public:
	// Called on environment step
	virtual void environment_Step(float dtime) {}

	// Called after generating a piece of map
	virtual void environment_OnGenerated(v3s16 minp, v3s16 maxp, u32 blockseed) {}

	// Called on player event
	virtual void player_event(ServerActiveObject *player, const std::string &type) {}

	// Called after emerge of a block queued from core.emerge_area()
	virtual void on_emerge_area_completion(v3s16 blockpos, int action, void *state) {}

   // Called after liquid transform changes
   virtual void on_liquid_transformed(const std::vector<std::pair<v3s16, MapNode>> &list) {}
};
}
}
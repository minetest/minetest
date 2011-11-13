#include "materials.h"
#include "mapnode.h"
#include "mapnode_contentfeatures.h"
#include "tool.h"

DiggingProperties getDiggingProperties(u16 material, ToolDiggingProperties *tp)
{
	assert(tp);
	MaterialProperties &mp = content_features(material).material;
	if(mp.diggability == DIGGABLE_NOT)
		return DiggingProperties(false, 0, 0);
	if(mp.diggability == DIGGABLE_CONSTANT)
		return DiggingProperties(true, mp.constant_time, 0);

	float time = tp->basetime;
	time += tp->dt_weight * mp.weight;
	time += tp->dt_crackiness * mp.crackiness;
	time += tp->dt_crumbliness * mp.crumbliness;
	time += tp->dt_cuttability * mp.cuttability;
	if(time < 0.2)
		time = 0.2;

	float durability = tp->basedurability;
	durability += tp->dd_weight * mp.weight;
	durability += tp->dd_crackiness * mp.crackiness;
	durability += tp->dd_crumbliness * mp.crumbliness;
	durability += tp->dd_cuttability * mp.cuttability;
	if(durability < 1)
		durability = 1;

	float wear = 1.0 / durability;
	u16 wear_i = wear/65535.;
	return DiggingProperties(true, time, wear_i);
}


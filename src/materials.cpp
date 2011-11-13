#include "materials.h"
#include "mapnode.h"
#include "mapnode_contentfeatures.h"


struct ToolProperties
{
	// time = basetime + sum(feature here * feature in MaterialProperties)
	float basetime;
	float dt_weight;
	float dt_crackiness;
	float dt_crumbliness;
	float dt_cuttability;
	float basedurability;
	float dd_weight;
	float dd_crackiness;
	float dd_crumbliness;
	float dd_cuttability;

	ToolProperties(float a=0.75, float b=0, float c=0, float d=0, float e=0,
			float f=50, float g=0, float h=0, float i=0, float j=0):
		basetime(a),
		dt_weight(b),
		dt_crackiness(c),
		dt_crumbliness(d),
		dt_cuttability(e),
		basedurability(f),
		dd_weight(g),
		dd_crackiness(h),
		dd_crumbliness(i),
		dd_cuttability(j)
	{}
};

ToolProperties getToolProperties(const std::string &toolname)
{
	// weight, crackiness, crumbleness, cuttability
	if(toolname == "WPick")
		return ToolProperties(2.0, 0,-1,2,0, 50, 0,0,0,0);
	else if(toolname == "STPick")
		return ToolProperties(1.5, 0,-1,2,0, 100, 0,0,0,0);
	else if(toolname == "SteelPick")
		return ToolProperties(1.0, 0,-1,2,0, 300, 0,0,0,0);

	else if(toolname == "MesePick")
		return ToolProperties(0, 0,0,0,0, 1337, 0,0,0,0);
	
	else if(toolname == "WShovel")
		return ToolProperties(1.5, 0.5,2,-1.5,0.3, 50, 0,0,0,0);
	else if(toolname == "STShovel")
		return ToolProperties(1.0, 0.5,2,-1.5,0.1, 100, 0,0,0,0);
	else if(toolname == "SteelShovel")
		return ToolProperties(0.6, 0.5,2,-1.5,0.0, 300, 0,0,0,0);

	// weight, crackiness, crumbleness, cuttability
	else if(toolname == "WAxe")
		return ToolProperties(2.0, 0.5,-0.2,1,-0.5, 50, 0,0,0,0);
	else if(toolname == "STAxe")
		return ToolProperties(1.5, 0.5,-0.2,1,-0.5, 100, 0,0,0,0);
	else if(toolname == "SteelAxe")
		return ToolProperties(1.0, 0.5,-0.2,1,-0.5, 300, 0,0,0,0);

	else if(toolname == "WSword")
		return ToolProperties(3.0, 3,0,1,-1, 50, 0,0,0,0);
	else if(toolname == "STSword")
		return ToolProperties(2.5, 3,0,1,-1, 100, 0,0,0,0);
	else if(toolname == "SteelSword")
		return ToolProperties(2.0, 3,0,1,-1, 300, 0,0,0,0);

	// Properties of hand
	return ToolProperties(0.5, 1,0.4,-0.75,0, 50, 0,0,0,0);
}

DiggingProperties getDiggingProperties(u16 material, const std::string &tool)
{
	MaterialProperties &mp = content_features(material).material;
	if(mp.diggability == DIGGABLE_NOT)
		return DiggingProperties(false, 0, 0);
	if(mp.diggability == DIGGABLE_CONSTANT)
		return DiggingProperties(true, mp.constant_time, 0);

	ToolProperties tp = getToolProperties(tool);
	
	float time = tp.basetime;
	time += tp.dt_weight * mp.weight;
	time += tp.dt_crackiness * mp.crackiness;
	time += tp.dt_crumbliness * mp.crumbliness;
	time += tp.dt_cuttability * mp.cuttability;
	if(time < 0.2)
		time = 0.2;

	float durability = tp.basedurability;
	durability += tp.dd_weight * mp.weight;
	durability += tp.dd_crackiness * mp.crackiness;
	durability += tp.dd_crumbliness * mp.crumbliness;
	durability += tp.dd_cuttability * mp.cuttability;
	if(durability < 1)
		durability = 1;

	float wear = 1.0 / durability;
	u16 wear_i = wear/65535.;
	return DiggingProperties(true, time, wear_i);
}


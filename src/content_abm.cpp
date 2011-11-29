/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "content_abm.h"

#include "environment.h"
#include "gamedef.h"
#include "nodedef.h"
#include "content_sao.h"
#include "settings.h"
#include "mapblock.h" // For getNodeBlockPos
#include "mapgen.h" // For mapgen::make_tree

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

class GrowGrassABM : public ActiveBlockModifier
{
private:
public:
	virtual std::set<std::string> getTriggerContents()
	{
		std::set<std::string> s;
		s.insert("dirt");
		return s;
	}
	virtual float getTriggerInterval()
	{ return 2.0; }
	virtual u32 getTriggerChance()
	{ return 200; }
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n)
	{
		INodeDefManager *ndef = env->getGameDef()->ndef();
		ServerMap *map = &env->getServerMap();
		
		MapNode n_top = map->getNodeNoEx(p+v3s16(0,1,0));
		if(ndef->get(n_top).light_propagates &&
				!ndef->get(n_top).isLiquid() &&
				n_top.getLightBlend(env->getDayNightRatio(), ndef) >= 13)
		{
			n.setContent(ndef->getId("dirt_with_grass"));
			map->addNodeWithEvent(p, n);
		}
	}
};

class RemoveGrassABM : public ActiveBlockModifier
{
private:
public:
	virtual std::set<std::string> getTriggerContents()
	{
		std::set<std::string> s;
		s.insert("dirt_with_grass");
		return s;
	}
	virtual float getTriggerInterval()
	{ return 2.0; }
	virtual u32 getTriggerChance()
	{ return 20; }
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n)
	{
		INodeDefManager *ndef = env->getGameDef()->ndef();
		ServerMap *map = &env->getServerMap();
		
		MapNode n_top = map->getNodeNoEx(p+v3s16(0,1,0));
		if(!ndef->get(n_top).light_propagates ||
				ndef->get(n_top).isLiquid())
		{
			n.setContent(ndef->getId("dirt"));
			map->addNodeWithEvent(p, n);
		}
	}
};

class SpawnRatsAroundTreesABM : public ActiveBlockModifier
{
private:
public:
	virtual std::set<std::string> getTriggerContents()
	{
		std::set<std::string> s;
		s.insert("tree");
		s.insert("jungletree");
		return s;
	}
	virtual float getTriggerInterval()
	{ return 10.0; }
	virtual u32 getTriggerChance()
	{ return 200; }
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider)
	{
		if(active_object_count_wider != 0)
			return;

		INodeDefManager *ndef = env->getGameDef()->ndef();
		ServerMap *map = &env->getServerMap();
		
		v3s16 p1 = p + v3s16(myrand_range(-2, 2),
				0, myrand_range(-2, 2));
		MapNode n1 = map->getNodeNoEx(p1);
		MapNode n1b = map->getNodeNoEx(p1+v3s16(0,-1,0));
		if(n1b.getContent() == ndef->getId("dirt_with_grass") &&
				n1.getContent() == CONTENT_AIR)
		{
			v3f pos = intToFloat(p1, BS);
			ServerActiveObject *obj = new RatSAO(env, pos);
			env->addActiveObject(obj);
		}
	}
};

static void getMob_dungeon_master(Settings &properties)
{
	properties.set("looks", "dungeon_master");
	properties.setFloat("yaw", 1.57);
	properties.setFloat("hp", 30);
	properties.setBool("bright_shooting", true);
	properties.set("shoot_type", "fireball");
	properties.set("shoot_y", "0.7");
	properties.set("player_hit_damage", "1");
	properties.set("player_hit_distance", "1.0");
	properties.set("player_hit_interval", "0.5");
	properties.setBool("mindless_rage", myrand_range(0,100)==0);
}

class SpawnInCavesABM : public ActiveBlockModifier
{
private:
public:
	virtual std::set<std::string> getTriggerContents()
	{
		std::set<std::string> s;
		s.insert("stone");
		s.insert("mossycobble");
		return s;
	}
	virtual float getTriggerInterval()
	{ return 2.0; }
	virtual u32 getTriggerChance()
	{ return 1000; }
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider)
	{
		if(active_object_count_wider != 0)
			return;

		INodeDefManager *ndef = env->getGameDef()->ndef();
		ServerMap *map = &env->getServerMap();

		v3s16 p1 = p + v3s16(0,1,0);
		MapNode n1a = map->getNodeNoEx(p1+v3s16(0,0,0));
		if(n1a.getLightBlend(env->getDayNightRatio(), ndef) <= 3){
			MapNode n1b = map->getNodeNoEx(p1+v3s16(0,1,0));
			if(n1a.getContent() == CONTENT_AIR &&
					n1b.getContent() == CONTENT_AIR)
			{
				v3f pos = intToFloat(p1, BS);
				int i = myrand()%5;
				if(i == 0 || i == 1){
					actionstream<<"A dungeon master spawns at "
							<<PP(p1)<<std::endl;
					Settings properties;
					getMob_dungeon_master(properties);
					ServerActiveObject *obj = new MobV2SAO(
							env, pos, &properties);
					env->addActiveObject(obj);
				} else if(i == 2 || i == 3){
					actionstream<<"Rats spawn at "
							<<PP(p1)<<std::endl;
					for(int j=0; j<3; j++){
						ServerActiveObject *obj = new RatSAO(
								env, pos);
						env->addActiveObject(obj);
					}
				} else {
					actionstream<<"An oerkki spawns at "
							<<PP(p1)<<std::endl;
					ServerActiveObject *obj = new Oerkki1SAO(
							env, pos);
					env->addActiveObject(obj);
				}
			}
		}
	}
};

class MakeTreesFromSaplingsABM : public ActiveBlockModifier
{
private:
public:
	virtual std::set<std::string> getTriggerContents()
	{
		std::set<std::string> s;
		s.insert("sapling");
		return s;
	}
	virtual float getTriggerInterval()
	{ return 10.0; }
	virtual u32 getTriggerChance()
	{ return 50; }
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider)
	{
		INodeDefManager *ndef = env->getGameDef()->ndef();
		ServerMap *map = &env->getServerMap();
		
		actionstream<<"A sapling grows into a tree at "
				<<PP(p)<<std::endl;

		core::map<v3s16, MapBlock*> modified_blocks;
		v3s16 tree_p = p;
		ManualMapVoxelManipulator vmanip(map);
		v3s16 tree_blockp = getNodeBlockPos(tree_p);
		vmanip.initialEmerge(tree_blockp - v3s16(1,1,1), tree_blockp + v3s16(1,1,1));
		bool is_apple_tree = myrand()%4 == 0;
		mapgen::make_tree(vmanip, tree_p, is_apple_tree, ndef);
		vmanip.blitBackAll(&modified_blocks);

		// update lighting
		core::map<v3s16, MapBlock*> lighting_modified_blocks;
		for(core::map<v3s16, MapBlock*>::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
		{
			lighting_modified_blocks.insert(i.getNode()->getKey(), i.getNode()->getValue());
		}
		map->updateLighting(lighting_modified_blocks, modified_blocks);

		// Send a MEET_OTHER event
		MapEditEvent event;
		event.type = MEET_OTHER;
		for(core::map<v3s16, MapBlock*>::Iterator
			i = modified_blocks.getIterator();
			i.atEnd() == false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			event.modified_blocks.insert(p, true);
		}
		map->dispatchEvent(&event);
	}
};

void add_legacy_abms(ServerEnvironment *env, INodeDefManager *nodedef)
{
	env->addActiveBlockModifier(new GrowGrassABM());
	env->addActiveBlockModifier(new RemoveGrassABM());
	env->addActiveBlockModifier(new SpawnRatsAroundTreesABM());
	env->addActiveBlockModifier(new SpawnInCavesABM());
	env->addActiveBlockModifier(new MakeTreesFromSaplingsABM());
}



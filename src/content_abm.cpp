/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
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
#include "treegen.h" // For treegen::make_tree
#include "main.h" // for g_settings
#include "map.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

class GrowGrassABM : public ActiveBlockModifier
{
private:
public:
	virtual std::set<std::string> getTriggerContents()
	{
		std::set<std::string> s;
		s.insert("mapgen_dirt");
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
			n.setContent(ndef->getId("mapgen_dirt_with_grass"));
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
		s.insert("mapgen_dirt_with_grass");
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
		if((!ndef->get(n_top).light_propagates &&
				n_top.getContent() != CONTENT_IGNORE) ||
				ndef->get(n_top).isLiquid())
		{
			n.setContent(ndef->getId("mapgen_dirt"));
			map->addNodeWithEvent(p, n);
		}
	}
};

class MakeTreesFromSaplingsABM : public ActiveBlockModifier
{
private:
	content_t c_junglesapling;
	content_t c_dirt;
	content_t c_dirt_with_grass;
	
public:
	MakeTreesFromSaplingsABM(ServerEnvironment *env, INodeDefManager *nodemgr) {
		c_junglesapling   = nodemgr->getId("junglesapling");
		c_dirt            = nodemgr->getId("mapgen_dirt");
		c_dirt_with_grass = nodemgr->getId("mapgen_dirt_with_grass");
	}

	virtual std::set<std::string> getTriggerContents()
	{
		std::set<std::string> s;
		s.insert("sapling");
		s.insert("junglesapling");
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
		
		MapNode n_below = map->getNodeNoEx(p - v3s16(0, 1, 0));
		if (n_below.getContent() != c_dirt &&
			n_below.getContent() != c_dirt_with_grass)
			return;
			
		bool is_jungle_tree = n.getContent() == c_junglesapling;
		
		actionstream <<"A " << (is_jungle_tree ? "jungle " : "")
				<< "sapling grows into a tree at "
				<< PP(p) << std::endl;

		std::map<v3s16, MapBlock*> modified_blocks;
		v3s16 tree_p = p;
		ManualMapVoxelManipulator vmanip(map);
		v3s16 tree_blockp = getNodeBlockPos(tree_p);
		vmanip.initialEmerge(tree_blockp - v3s16(1,1,1), tree_blockp + v3s16(1,1,1));
		
		if (is_jungle_tree) {
			treegen::make_jungletree(vmanip, tree_p, ndef, myrand());
		} else {
			bool is_apple_tree = myrand() % 4 == 0;
			treegen::make_tree(vmanip, tree_p, is_apple_tree, ndef, myrand());
		}
		
		vmanip.blitBackAll(&modified_blocks);

		// update lighting
		std::map<v3s16, MapBlock*> lighting_modified_blocks;
		lighting_modified_blocks.insert(modified_blocks.begin(), modified_blocks.end());
		map->updateLighting(lighting_modified_blocks, modified_blocks);

		// Send a MEET_OTHER event
		MapEditEvent event;
		event.type = MEET_OTHER;
//		event.modified_blocks.insert(modified_blocks.begin(), modified_blocks.end());
		for(std::map<v3s16, MapBlock*>::iterator
			i = modified_blocks.begin();
			i != modified_blocks.end(); ++i)
		{
			event.modified_blocks.insert(i->first);
		}
		map->dispatchEvent(&event);
	}
};

class LiquidFlowABM : public ActiveBlockModifier
{
private:
	std::set<std::string> contents;

public:
	LiquidFlowABM(ServerEnvironment *env, INodeDefManager *nodemgr) 
	{
		std::set<content_t> liquids;
		nodemgr->getIds("group:liquid", liquids);
		for(std::set<content_t>::const_iterator k = liquids.begin(); k != liquids.end(); k++)
			contents.insert(nodemgr->get(*k).liquid_alternative_flowing);
		
	}
	virtual std::set<std::string> getTriggerContents()
	{
		return contents;
	}
	virtual float getTriggerInterval()
	{ return 10.0; }
	virtual u32 getTriggerChance()
	{ return 10; }
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n)
	{
		ServerMap *map = &env->getServerMap();
		if (map->transforming_liquid_size() < 500)
			map->transforming_liquid_add(p);
		//if ((*map).m_transforming_liquid.size() < 500) (*map).m_transforming_liquid.push_back(p);
	}
};

void add_legacy_abms(ServerEnvironment *env, INodeDefManager *nodedef)
{
	env->addActiveBlockModifier(new GrowGrassABM());
	env->addActiveBlockModifier(new RemoveGrassABM());
	env->addActiveBlockModifier(new MakeTreesFromSaplingsABM(env, nodedef));
	if (g_settings->getBool("liquid_finite"))
		env->addActiveBlockModifier(new LiquidFlowABM(env, nodedef));
}

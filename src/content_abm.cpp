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
#include "scripting_game.h"
#include "log.h"

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
		content_t c_snow = ndef->getId("snow");
		if(ndef->get(n_top).light_propagates &&
				!ndef->get(n_top).isLiquid() &&
				n_top.getLightBlend(env->getDayNightRatio(), ndef) >= 13)
		{
			if(c_snow != CONTENT_IGNORE && n_top.getContent() == c_snow)
				n.setContent(ndef->getId("dirt_with_snow"));
			else
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
	
public:
	MakeTreesFromSaplingsABM(ServerEnvironment *env, INodeDefManager *nodemgr) {
		c_junglesapling = nodemgr->getId("junglesapling");
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
		if (!((ItemGroupList) ndef->get(n_below).groups)["soil"])
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

class LiquidFlowABM : public ActiveBlockModifier {
	private:
		std::set<std::string> contents;

	public:
		LiquidFlowABM(ServerEnvironment *env, INodeDefManager *nodemgr) {
			std::set<content_t> liquids;
			nodemgr->getIds("group:liquid", liquids);
			for(std::set<content_t>::const_iterator k = liquids.begin(); k != liquids.end(); k++)
				contents.insert(nodemgr->get(*k).liquid_alternative_flowing);
		}
		virtual std::set<std::string> getTriggerContents() {
			return contents;
		}
		virtual float getTriggerInterval()
		{ return 10.0; }
		virtual u32 getTriggerChance()
		{ return 10; }
		virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n) {
			ServerMap *map = &env->getServerMap();
			if (map->transforming_liquid_size() > 500)
				return;
			map->transforming_liquid_add(p);
		}
};

class LiquidDropABM : public ActiveBlockModifier {
	private:
		std::set<std::string> contents;

	public:
		LiquidDropABM(ServerEnvironment *env, INodeDefManager *nodemgr) {
			std::set<content_t> liquids;
			nodemgr->getIds("group:liquid", liquids);
			for(std::set<content_t>::const_iterator k = liquids.begin(); k != liquids.end(); k++)
				contents.insert(nodemgr->get(*k).liquid_alternative_source);
		}
		virtual std::set<std::string> getTriggerContents()
		{ return contents; }
		virtual std::set<std::string> getRequiredNeighbors() {
			std::set<std::string> neighbors;
			neighbors.insert("air");
			return neighbors;
		}
		virtual float getTriggerInterval()
		{ return 20.0; }
		virtual u32 getTriggerChance()
		{ return 10; }
		virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n) {
			ServerMap *map = &env->getServerMap();
			if (map->transforming_liquid_size() > 500)
				return;
			if (   map->getNodeNoEx(p - v3s16(0,  1, 0 )).getContent() != CONTENT_AIR  // below
			    && map->getNodeNoEx(p - v3s16(1,  0, 0 )).getContent() != CONTENT_AIR  // right
			    && map->getNodeNoEx(p - v3s16(-1, 0, 0 )).getContent() != CONTENT_AIR  // left
			    && map->getNodeNoEx(p - v3s16(0,  0, 1 )).getContent() != CONTENT_AIR  // back
			    && map->getNodeNoEx(p - v3s16(0,  0, -1)).getContent() != CONTENT_AIR  // front
			   )
				return;
			map->transforming_liquid_add(p);
		}
};

class LiquidFreeze : public ActiveBlockModifier {
	public:
		LiquidFreeze(ServerEnvironment *env, INodeDefManager *nodemgr) { }
		virtual std::set<std::string> getTriggerContents() {
			std::set<std::string> s;
			s.insert("group:freezes");
			return s;
		}
		virtual std::set<std::string> getRequiredNeighbors() {
			std::set<std::string> s;
			s.insert("air");
			s.insert("group:melts");
			return s;
		}
		virtual float getTriggerInterval()
		{ return 10.0; }
		virtual u32 getTriggerChance()
		{ return 20; }
		virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n) {
			ServerMap *map = &env->getServerMap();
			INodeDefManager *ndef = env->getGameDef()->ndef();

			float heat = map->updateBlockHeat(env, p);
			//heater = rare
			content_t c = map->getNodeNoEx(p - v3s16(0,  -1, 0 )).getContent(); // top
			//more chance to freeze if air at top
			if (heat <= -1 && (heat <= -50 || (myrand_range(-50, heat) <= (c == CONTENT_AIR ? -10 : -40)))) {
				content_t c_self = n.getContent();
				// making freeze not annoying, do not freeze random blocks in center of ocean
				// todo: any block not water (dont freeze _source near _flowing)
				bool allow = heat < -40;
				// todo: make for(...)
				if (!allow) {
				 c = map->getNodeNoEx(p - v3s16(0,  1, 0 )).getContent(); // below
				 if (c == CONTENT_AIR || c == CONTENT_IGNORE)
					return; // do not freeze when falling
				 if (c != c_self && c != CONTENT_IGNORE) allow = 1;
				 if (!allow) {
				  c = map->getNodeNoEx(p - v3s16(1,  0, 0 )).getContent(); // right
				  if (c != c_self && c != CONTENT_IGNORE) allow = 1;
				  if (!allow) {
				   c = map->getNodeNoEx(p - v3s16(-1, 0, 0 )).getContent(); // left
				   if (c != c_self && c != CONTENT_IGNORE) allow = 1;
				   if (!allow) {
				    c = map->getNodeNoEx(p - v3s16(0,  0, 1 )).getContent(); // back
				    if (c != c_self && c != CONTENT_IGNORE) allow = 1;
				    if (!allow) {
				     c = map->getNodeNoEx(p - v3s16(0,  0, -1)).getContent(); // front
				     if (c != c_self && c != CONTENT_IGNORE) allow = 1;
				    }
				   }
				  }
				 }
				}
				if (allow) {
					n.freezeMelt(ndef);
					map->addNodeWithEvent(p, n);
				}
			}
		}
};

class LiquidMeltWeather : public ActiveBlockModifier {
	public:
		LiquidMeltWeather(ServerEnvironment *env, INodeDefManager *nodemgr) { }
		virtual std::set<std::string> getTriggerContents() {
			std::set<std::string> s;
			s.insert("group:melts");
			return s;
		}
		virtual std::set<std::string> getRequiredNeighbors() {
			std::set<std::string> s;
			s.insert("air");
			s.insert("group:freezes");
			return s;
		}
		virtual float getTriggerInterval()
		{ return 10.0; }
		virtual u32 getTriggerChance()
		{ return 20; }
		virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n) {
			ServerMap *map = &env->getServerMap();
			INodeDefManager *ndef = env->getGameDef()->ndef();

			float heat = map->updateBlockHeat(env, p);
			content_t c = map->getNodeNoEx(p - v3s16(0,  -1, 0 )).getContent(); // top
			if (heat >= 1 && (heat >= 40 || ((myrand_range(heat, 40)) >= (c == CONTENT_AIR ? 10 : 20)))) {
				n.freezeMelt(ndef);
				map->addNodeWithEvent(p, n);
				env->getScriptIface()->node_falling_update(p);
			}
		}
};

class LiquidMeltHot : public ActiveBlockModifier {
	public:
		LiquidMeltHot(ServerEnvironment *env, INodeDefManager *nodemgr) { }
		virtual std::set<std::string> getTriggerContents() {
			std::set<std::string> s;
			s.insert("group:melts");
			return s;
		}
		virtual std::set<std::string> getRequiredNeighbors() {
			std::set<std::string> s;
			s.insert("group:igniter");
			s.insert("group:hot");
			return s;
		}
		virtual float getTriggerInterval()
		{ return 2.0; }
		virtual u32 getTriggerChance()
		{ return 4; }
		virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n) {
			ServerMap *map = &env->getServerMap();
			INodeDefManager *ndef = env->getGameDef()->ndef();
			n.freezeMelt(ndef);
			map->addNodeWithEvent(p, n);
			env->getScriptIface()->node_falling_update(p);
		}
};

/* too buggy, later via liquid flow code
class LiquidMeltAround : public LiquidMeltHot {
	public:
		LiquidMeltAround(ServerEnvironment *env, INodeDefManager *nodemgr) 
			: LiquidMeltHot(env, nodemgr) { }
		virtual std::set<std::string> getRequiredNeighbors() {
			std::set<std::string> s;
			s.insert("group:melt_around");
			return s;
		}
		virtual float getTriggerInterval()
		{ return 40.0; }
		virtual u32 getTriggerChance()
		{ return 60; }
};
*/

void add_legacy_abms(ServerEnvironment *env, INodeDefManager *nodedef) {
	env->addActiveBlockModifier(new GrowGrassABM());
	env->addActiveBlockModifier(new RemoveGrassABM());
	env->addActiveBlockModifier(new MakeTreesFromSaplingsABM(env, nodedef));
	if (g_settings->getBool("liquid_finite")) {
		env->addActiveBlockModifier(new LiquidFlowABM(env, nodedef));
		env->addActiveBlockModifier(new LiquidDropABM(env, nodedef));
		env->addActiveBlockModifier(new LiquidMeltHot(env, nodedef));
		//env->addActiveBlockModifier(new LiquidMeltAround(env, nodedef));
		if (env->m_use_weather) {
			env->addActiveBlockModifier(new LiquidFreeze(env, nodedef));
			env->addActiveBlockModifier(new LiquidMeltWeather(env, nodedef));
		}
	}
}

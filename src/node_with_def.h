/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef NODE_WITH_DEF_HEADER
#define NODE_WITH_DEF_HEADER

#include "mapnode.h"
#include "util/pointer.h"

struct ContentFeatures;
class INodeDefManager;

class NodeWithDef : public MapNode
{
	MapNode m_n;
	HybridPtr<const ContentFeatures> m_def;

public:
	NodeWithDef();
	NodeWithDef(const MapNode &n, const INodeDefManager *ndef);
	NodeWithDef(const MapNode &n, const HybridPtr<const ContentFeatures> &def);

	const MapNode & node() const
	{
		return *this;
	}
	const ContentFeatures * def() const
	{
		return m_def.get();
	}
	bool def_is_global() const
	{
		return m_def.getNeverDelete();
	}

	void setNode(const MapNode &n)
	{
		param0 = n.param0;
		param1 = n.param1;
		param2 = n.param2;
	}

	void setLight(enum LightBank bank, u8 a_light);
	u8 getLight(enum LightBank bank) const;
	bool getLightBanks(u8 &lightday, u8 &lightnight) const;
	
	// 0 <= daylight_factor <= 1000
	// 0 <= return value <= LIGHT_SUN
	u8 getLightBlend(u32 daylight_factor) const;

	u8 getFaceDir() const;
	u8 getWallMounted() const;
	v3s16 getWallMountedDir() const;

	// Get list of node boxes (used for rendering NDT_NODEBOX and collision)
	std::vector<aabb3f> getNodeBoxes() const;

	// Get list of selection boxes
	std::vector<aabb3f> getSelectionBoxes() const;
};



#endif

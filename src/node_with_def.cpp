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

#include "node_with_def.h"

#include "nodedef.h"

NodeWithDef::NodeWithDef()
{
}
NodeWithDef::NodeWithDef(const MapNode &n, const INodeDefManager *ndef):
	MapNode(n),
	m_def(NULL)
{
	m_def = &ndef->get(n);
	m_def.setNeverDelete(true);
}
NodeWithDef::NodeWithDef(const MapNode &n, const HybridPtr<const ContentFeatures> &def):
	MapNode(n),
	m_def(def)
{
}

void NodeWithDef::setLight(enum LightBank bank, u8 a_light)
{
	const ContentFeatures &f = *m_def;
	MapNode::setLight(bank, a_light, f);
}

u8 NodeWithDef::getLight(enum LightBank bank) const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getLight(bank, f);
}

bool NodeWithDef::getLightBanks(u8 &lightday, u8 &lightnight) const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getLightBanks(lightday, lightnight, f);
}

u8 NodeWithDef::getLightBlend(u32 daylight_factor) const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getLightBlend(daylight_factor, f);
}

u8 NodeWithDef::getFaceDir() const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getFaceDir(f);
}

u8 NodeWithDef::getWallMounted() const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getWallMounted(f);
}

v3s16 NodeWithDef::getWallMountedDir() const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getWallMountedDir(f);
}

std::vector<aabb3f> NodeWithDef::getNodeBoxes() const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getNodeBoxes(f);
}

std::vector<aabb3f> NodeWithDef::getSelectionBoxes() const
{
	const ContentFeatures &f = *m_def;
	return MapNode::getSelectionBoxes(f);
}


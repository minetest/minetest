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

#include "test.h"

#include "gamedef.h"
#include "nodedef.h"
#include "content_mapnode.h"

class TestMapNode : public TestBase
{
public:
	TestMapNode() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestMapNode"; }

	void runTests(IGameDef *gamedef);

	void testNodeProperties(const NodeDefManager *nodedef);
};

static TestMapNode g_test_instance;

void TestMapNode::runTests(IGameDef *gamedef)
{
	TEST(testNodeProperties, gamedef->getNodeDefManager());
}

////////////////////////////////////////////////////////////////////////////////

void TestMapNode::testNodeProperties(const NodeDefManager *nodedef)
{
	MapNode n(CONTENT_AIR);

	ContentLightingFlags f = nodedef->getLightingFlags(n);
	UASSERT(n.getContent() == CONTENT_AIR);
	UASSERT(n.getLight(LIGHTBANK_DAY, f) == 0);
	UASSERT(n.getLight(LIGHTBANK_NIGHT, f) == 0);

	// Transparency
	n.setContent(CONTENT_AIR);
	UASSERT(nodedef->get(n).light_propagates == true);
}

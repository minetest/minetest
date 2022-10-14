/*
Minetest
Copyright (C) 2022 Minetest core developers & community

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
#include "test_config.h"

#include "mock_inventorymanager.h"
#include "mock_server.h"
#include "mock_serveractiveobject.h"

#include <scripting_server.h>


class TestMoveAction : public TestBase
{
public:
	TestMoveAction() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestMoveAction"; }

	void runTests(IGameDef *gamedef);

	void testMove(ServerActiveObject *obj, IGameDef *gamedef);
	void testMoveSomewhere(ServerActiveObject *obj, IGameDef *gamedef);
	void testMoveUnallowed(ServerActiveObject *obj, IGameDef *gamedef);
	void testMovePartial(ServerActiveObject *obj, IGameDef *gamedef);
	void testSwap(ServerActiveObject *obj, IGameDef *gamedef);
	void testSwapFromUnallowed(ServerActiveObject *obj, IGameDef *gamedef);
	void testSwapToUnallowed(ServerActiveObject *obj, IGameDef *gamedef);
};

static TestMoveAction g_test_instance;

void TestMoveAction::runTests(IGameDef *gamedef)
{
	MockServer server;

	ServerScripting server_scripting(&server);
	server_scripting.loadMod(Server::getBuiltinLuaPath() + DIR_DELIM "init.lua", BUILTIN_MOD_NAME);
	server_scripting.loadMod(std::string(HELPERS_PATH) + DIR_DELIM "helper_moveaction.lua",	BUILTIN_MOD_NAME);

	MetricsBackend mb;
	ServerEnvironment server_env(nullptr, &server_scripting, &server, "", &mb);
	MockServerActiveObject obj(&server_env);

	TEST(testMove, &obj, gamedef);
	TEST(testMoveSomewhere, &obj, gamedef);
	TEST(testMoveUnallowed, &obj, gamedef);
	TEST(testMovePartial, &obj, gamedef);
	TEST(testSwap, &obj, gamedef);
	TEST(testSwapFromUnallowed, &obj, gamedef);
	TEST(testSwapToUnallowed, &obj, gamedef);
}

static ItemStack parse_itemstack(const char *s)
{
	ItemStack item;
	item.deSerialize(s, nullptr);
	return item;
}

static void apply_action(const char *s, InventoryManager *inv, ServerActiveObject *obj, IGameDef *gamedef)
{
	std::istringstream str(s);
	InventoryAction *action = InventoryAction::deSerialize(str);
	action->apply(inv, obj, gamedef);
	delete action;
}

void TestMoveAction::testMove(ServerActiveObject *obj, IGameDef *gamedef)
{
	MockInventoryManager inv(gamedef);

	inv.p1.addList("main", 10)->addItem(0, parse_itemstack("default:stone 50"));
	inv.p2.addList("main", 10);

	apply_action("Move 20 player:p1 main 0 player:p2 main 0", &inv, obj, gamedef);

	UASSERT(inv.p1.getList("main")->getItem(0).getItemString() == "default:stone 30");
	UASSERT(inv.p2.getList("main")->getItem(0).getItemString() == "default:stone 20");
}

void TestMoveAction::testMoveSomewhere(ServerActiveObject *obj, IGameDef *gamedef)
{
	MockInventoryManager inv(gamedef);

	inv.p1.addList("main", 10)->addItem(0, parse_itemstack("default:stone 50"));
	InventoryList *list = inv.p2.addList("main", 10);
	list->addItem(0, parse_itemstack("default:brick 10"));
	list->addItem(2, parse_itemstack("default:stone 85"));

	apply_action("MoveSomewhere 50 player:p1 main 0 player:p2 main", &inv, obj, gamedef);

	UASSERT(inv.p2.getList("main")->getItem(0).getItemString() == "default:brick 10");
	UASSERT(inv.p2.getList("main")->getItem(1).getItemString() == "default:stone 36");
	UASSERT(inv.p2.getList("main")->getItem(2).getItemString() == "default:stone 99");
}

void TestMoveAction::testMoveUnallowed(ServerActiveObject *obj, IGameDef *gamedef)
{
	MockInventoryManager inv(gamedef);

	inv.p1.addList("main", 10)->addItem(0, parse_itemstack("default:water 50"));
	inv.p2.addList("main", 10);

	apply_action("Move 20 player:p1 main 0 player:p2 main 0", &inv, obj, gamedef);

	UASSERT(inv.p1.getList("main")->getItem(0).getItemString() == "default:water 50");
	UASSERT(inv.p2.getList("main")->getItem(0).empty())
}

void TestMoveAction::testMovePartial(ServerActiveObject *obj, IGameDef *gamedef)
{
	MockInventoryManager inv(gamedef);

	inv.p1.addList("main", 10)->addItem(0, parse_itemstack("default:lava 50"));
	inv.p2.addList("main", 10);

	apply_action("Move 20 player:p1 main 0 player:p2 main 0", &inv, obj, gamedef);

	UASSERT(inv.p1.getList("main")->getItem(0).getItemString() == "default:lava 45");
	UASSERT(inv.p2.getList("main")->getItem(0).getItemString() == "default:lava 5");
}

void TestMoveAction::testSwap(ServerActiveObject *obj, IGameDef *gamedef)
{
	MockInventoryManager inv(gamedef);

	inv.p1.addList("main", 10)->addItem(0, parse_itemstack("default:stone 50"));
	inv.p2.addList("main", 10)->addItem(0, parse_itemstack("default:brick 60"));

	apply_action("Move 50 player:p1 main 0 player:p2 main 0", &inv, obj, gamedef);

	UASSERT(inv.p1.getList("main")->getItem(0).getItemString() == "default:brick 60");
	UASSERT(inv.p2.getList("main")->getItem(0).getItemString() == "default:stone 50");
}

void TestMoveAction::testSwapFromUnallowed(ServerActiveObject *obj, IGameDef *gamedef)
{
	MockInventoryManager inv(gamedef);

	inv.p1.addList("main", 10)->addItem(0, parse_itemstack("default:water 50"));
	inv.p2.addList("main", 10)->addItem(0, parse_itemstack("default:brick 60"));

	apply_action("Move 50 player:p1 main 0 player:p2 main 0", &inv, obj, gamedef);

	UASSERT(inv.p1.getList("main")->getItem(0).getItemString() == "default:water 50");
	UASSERT(inv.p2.getList("main")->getItem(0).getItemString() == "default:brick 60");
}

void TestMoveAction::testSwapToUnallowed(ServerActiveObject *obj, IGameDef *gamedef)
{
	MockInventoryManager inv(gamedef);

	inv.p1.addList("main", 10)->addItem(0, parse_itemstack("default:stone 50"));
	inv.p2.addList("main", 10)->addItem(0, parse_itemstack("default:water 60"));

	apply_action("Move 50 player:p1 main 0 player:p2 main 0", &inv, obj, gamedef);

	UASSERT(inv.p1.getList("main")->getItem(0).getItemString() == "default:stone 50");
	UASSERT(inv.p2.getList("main")->getItem(0).getItemString() == "default:water 60");
}

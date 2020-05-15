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

#include <sstream>

#include "gamedef.h"
#include "inventory.h"

class TestInventory : public TestBase {
public:
	TestInventory() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestInventory"; }

	void runTests(IGameDef *gamedef);

	void testSerializeDeserialize(IItemDefManager *idef);
	void testItemStackComparison();

	static const char *serialized_inventory_in;
	static const char *serialized_inventory_out;
	static const char *serialized_inventory_inc;
};

static TestInventory g_test_instance;

void TestInventory::runTests(IGameDef *gamedef)
{
	TEST(testSerializeDeserialize, gamedef->getItemDefManager());
	TEST(testItemStackComparison);
}

////////////////////////////////////////////////////////////////////////////////

void TestInventory::testSerializeDeserialize(IItemDefManager *idef)
{
	Inventory inv(idef);
	std::istringstream is(serialized_inventory_in, std::ios::binary);

	inv.deSerialize(is);
	UASSERT(inv.getList("0"));
	UASSERT(!inv.getList("main"));

	inv.getList("0")->setName("main");
	UASSERT(!inv.getList("0"));
	UASSERT(inv.getList("main"));
	UASSERTEQ(u32, inv.getList("main")->getWidth(), 3);

	inv.getList("main")->setWidth(5);
	std::ostringstream inv_os(std::ios::binary);
	inv.serialize(inv_os, false);
	UASSERTEQ(std::string, inv_os.str(), serialized_inventory_out);

	inv.setModified(false);
	inv_os.str("");
	inv_os.clear();
	inv.serialize(inv_os, true);
	UASSERTEQ(std::string, inv_os.str(), serialized_inventory_inc);

	ItemStack leftover = inv.getList("main")->takeItem(7, 99 - 12);
	ItemStack wanted = ItemStack("default:dirt", 99 - 12, 0, idef);
	UASSERT(leftover == wanted);
	leftover = inv.getList("main")->getItem(7);
	wanted.count = 12;
	UASSERT(leftover == wanted);
}

void TestInventory::testItemStackComparison()
{
	ItemStack stack_a, stack_b;
	UASSERT(stack_a == stack_b);

	ItemStackMetadata meta_a, meta_b;
	sanity_check(meta_a.setString("key", "meta_a"));
	sanity_check(meta_b.setString("key", "meta_b"));

	stack_a.name = "stack_a";
	stack_b.name = "stack_b";
	UASSERT(stack_a != stack_b);

	stack_a.name  = stack_b.name  = "stack";
	stack_a.count = stack_b.count = 42;
	stack_a.wear  = stack_b.wear  = 666;
	stack_a.metadata = meta_a;
	stack_b.metadata = meta_b;
	UASSERT(stack_a != stack_b);

	stack_b.metadata = meta_a;
	UASSERT(stack_a.metadata == stack_b.metadata);
	UASSERT(stack_a == stack_b);
}

const char *TestInventory::serialized_inventory_in =
	"List 0 10\n"
	"Width 3\n"
	"Empty\n"
	"Empty\n"
	"Item default:cobble 61\n"
	"Empty\n"
	"Empty\n"
	"Item default:dirt 71\n"
	"Empty\n"
	"Item default:dirt 99\n"
	"Item default:cobble 38\n"
	"Empty\n"
	"EndInventoryList\n"
	"List abc 1\n"
	"Item default:stick 3\n"
	"Width 0\n"
	"EndInventoryList\n"
	"EndInventory\n";

const char *TestInventory::serialized_inventory_out =
	"List main 10\n"
	"Width 5\n"
	"Empty\n"
	"Empty\n"
	"Item default:cobble 61\n"
	"Empty\n"
	"Empty\n"
	"Item default:dirt 71\n"
	"Empty\n"
	"Item default:dirt 99\n"
	"Item default:cobble 38\n"
	"Empty\n"
	"EndInventoryList\n"
	"List abc 1\n"
	"Width 0\n"
	"Item default:stick 3\n"
	"EndInventoryList\n"
	"EndInventory\n";

const char *TestInventory::serialized_inventory_inc =
	"KeepList main\n"
	"KeepList abc\n"
	"EndInventory\n";

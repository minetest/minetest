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
#include "itemstackmetadata.h"

class TestInventory : public TestBase {
public:
	TestInventory() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestInventory"; }

	void runTests(IGameDef *gamedef);

	void testSerializeDeserialize(IItemDefManager *idef);
	void testDeSerializeItemStackMetadata();
	void testSerializeDeserializeItemMetadata();

	static const char *serialized_inventory;
	static const char *serialized_inventory_2;
};

static TestInventory g_test_instance;

void TestInventory::runTests(IGameDef *gamedef)
{
	TEST(testSerializeDeserialize, gamedef->getItemDefManager());
	TEST(testDeSerializeItemStackMetadata);
	TEST(testSerializeDeserializeItemMetadata);
}

////////////////////////////////////////////////////////////////////////////////

void TestInventory::testSerializeDeserialize(IItemDefManager *idef)
{
	Inventory inv(idef);
	std::istringstream is(serialized_inventory, std::ios::binary);

	inv.deSerialize(is);
	UASSERT(inv.getList("0"));
	UASSERT(!inv.getList("main"));

	inv.getList("0")->setName("main");
	UASSERT(!inv.getList("0"));
	UASSERT(inv.getList("main"));
	UASSERTEQ(u32, inv.getList("main")->getWidth(), 3);

	inv.getList("main")->setWidth(5);
	std::ostringstream inv_os(std::ios::binary);
	inv.serialize(inv_os);
	UASSERTEQ(std::string, inv_os.str(), serialized_inventory_2);
}

const char *TestInventory::serialized_inventory =
	"List 0 32\n"
	"Width 3\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Item default:cobble 61\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Item default:dirt 71\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Item default:dirt 99\n"
	"Item default:cobble 38\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"EndInventoryList\n"
	"EndInventory\n";

const char *TestInventory::serialized_inventory_2 =
	"List main 32\n"
	"Width 5\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Item default:cobble 61\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Item default:dirt 71\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Item default:dirt 99\n"
	"Item default:cobble 38\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"Empty\n"
	"EndInventoryList\n"
	"EndInventory\n";

#include "util/serialize.h"

#define DESERIALIZE_START '\x01'
#define DESERIALIZE_KV_DELIM '\x02'
#define DESERIALIZE_PAIR_DELIM '\x03'
#define DESERIALIZE_START_STR "\x01"
#define DESERIALIZE_KV_DELIM_STR "\x02"
#define DESERIALIZE_PAIR_DELIM_STR "\x03"
#define SERIALIZATION_VERSION_IDENTIFIER '\x02'

void TestInventory::testDeSerializeItemStackMetadata() {
	{
		std::istringstream is(serializeJsonString("some meta\nasdkmskdmsds\nasasasas"));

		ItemStackMetadata metadata;
		metadata.deSerialize(is);

		UASSERT(metadata.size() == 1);
		UASSERT(metadata.getString("") == "some meta\nasdkmskdmsds\nasasasas");
	}

	{
		std::ostringstream is;
		is << DESERIALIZE_START
		   << "foo"
		   << DESERIALIZE_KV_DELIM
		   << "bar"
		   << DESERIALIZE_PAIR_DELIM
		   << "description"
		   << DESERIALIZE_KV_DELIM
		   << "A foo bar thing\nbleh"
		   << DESERIALIZE_PAIR_DELIM;

		std::istringstream is2(serializeJsonString(is.str()));

		ItemStackMetadata metadata;
		metadata.deSerialize(is2);

		UASSERT(metadata.size() == 2);
		UASSERT(metadata.getString("foo") == "bar");
		UASSERT(metadata.getString("description") == "A foo bar thing\nbleh");
	}

	{
		std::ostringstream is;
		is << SERIALIZATION_VERSION_IDENTIFIER
		   << "{ \"foo\": \"bar\", \"description\": \"this is a JSON string!\\nHow exciting!\"}";

		std::istringstream is2(is.str());

		ItemStackMetadata metadata;
		metadata.deSerialize(is2);

		UASSERT(metadata.size() == 2);
		UASSERT(metadata.getString("foo") == "bar");
		UASSERT(metadata.getString("description") == "this is a JSON string!\nHow exciting!");
	}

	{
		const std::string test = "{ \"foo\": \"bar\", \"description\": \"this is a JSON string!\\nHow exciting!\"}";

		std::istringstream is2(serializeJsonString(test));

		ItemStackMetadata metadata;
		metadata.deSerialize(is2);

		UASSERT(metadata.size() == 1);
		UASSERT(metadata.getString("") == test);
	}
}

void TestInventory::testSerializeDeserializeItemMetadata() {
	ItemStackMetadata metadata;
	metadata.setString("one", "two");
	metadata.setString("foo", "{\"\"sn4##434#sas#34eds#}");
	metadata.setString("bar", "\x01\x02\x03\x02\x03\x01");
	metadata.setString("boo", "{\"\"snsdfb sdf  sdfsdf {} sdjkffb sdf9347sdf \n sdfjjbsdf \t\r\n skdfjjkgbsdfjlgb \n");
	metadata.setString("\x01", "asasasas");


	std::ostringstream os;
	metadata.serialize(os);

	std::string serialisedString = os.str();

	UASSERT(serialisedString.find('\n') == std::string::npos);

	std::istringstream is(serialisedString);
	ItemStackMetadata metadata2;
	metadata2.deSerialize(is);

	UASSERT(metadata2 == metadata);
}

// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "test.h"

#include <sstream>
#include "gamedef.h"
#include "nodedef.h"
#include "mapblock.h"
#include "serialization.h"
#include "noise.h"
#include "inventory.h"

class TestMapBlock : public TestBase
{
public:
	TestMapBlock() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestMapBlock"; }

	void runTests(IGameDef *gamedef);

	void testSaveLoad(IGameDef *gamedef, u8 ver);
	inline void testSaveLoadLowest(IGameDef *gamedef) {
		testSaveLoad(gamedef, SER_FMT_VER_LOWEST_WRITE);
	}

	void testLoad29(IGameDef *gamedef);
};

static TestMapBlock g_test_instance;

void TestMapBlock::runTests(IGameDef *gamedef)
{
	TEST(testSaveLoad, gamedef, SER_FMT_VER_HIGHEST_WRITE);
	TEST(testSaveLoadLowest, gamedef);
	TEST(testLoad29, gamedef);
}

////////////////////////////////////////////////////////////////////////////////

void TestMapBlock::testSaveLoad(IGameDef *gamedef, const u8 version)
{
	// Use the bottom node ids for this test
	content_t max = 0;
	{
		auto *ndef = gamedef->getNodeDefManager();
		const auto &unknown_name = ndef->get(CONTENT_UNKNOWN).name;
		while (ndef->get(max).name != unknown_name)
			max++;
	}
	UASSERT(max >= 2);
	UASSERT(max <= CONTENT_UNKNOWN);

	constexpr u64 seed = 0x207366616e3520ULL;
	std::stringstream ss;
	{
		MapBlock block({}, gamedef);
		// Fill with data
		PcgRandom r(seed);
		for (size_t i = 0; i < MapBlock::nodecount; ++i) {
			u32 rval = r.next();
			block.getData()[i] =
				MapNode(rval % max, (rval >> 16) & 0xff, (rval >> 24) & 0xff);
		}

		// Serialize
		block.serialize(ss, version, true, -1);
	}

	{
		MapBlock block({}, gamedef);
		// Deserialize
		block.deSerialize(ss, version, true);

		// Check data
		PcgRandom r(seed);
		for (size_t i = 0; i < MapBlock::nodecount; ++i) {
			u32 rval = r.next();
			auto expect =
				MapNode(rval % max, (rval >> 16) & 0xff, (rval >> 24) & 0xff);
			UASSERT(block.getData()[i] == expect);
		}
	}
}

// This was generated with: minetestmapper -i testworld --dumpblock 6,0,0 |
// python -c 'import sys;d=bytes.fromhex(sys.stdin.read().strip());print(",".join("%d"%c for c in d))'

static const u8 coded_mapblock29[] = {
	29,40,181,47,253,0,88,77,11,0,2,138,32,38,160,23,36,29,255,255,103,85,85,
	213,136,105,255,110,103,27,53,43,218,89,210,205,184,194,207,24,226,254,135,
	7,16,191,92,232,249,141,144,20,175,126,149,175,29,143,211,164,99,16,206,80,
	28,115,77,190,197,176,52,212,147,14,12,130,34,88,75,42,45,65,21,24,44,215,
	16,102,229,107,199,191,192,232,52,26,47,182,136,216,22,91,108,177,45,182,
	88,44,98,199,127,148,30,253,41,49,36,205,81,120,167,33,197,232,124,108,73,
	14,178,225,1,89,96,74,192,233,226,7,34,0,78,36,138,124,168,193,182,41,40,42,
	148,182,14,48,131,17,74,155,12,99,18,0,113,80,150,98,137,8,98,56,144,8,20,
	33,66,66,136,128,8,8,17,32,225,7,205,42,26,14,52,88,86,216,141,181,243,179,
	3,4,215,245,139,22,83,172,157,47,73,82,216,114,3,60,78,223,0,155,247,80,5,
	12,253,12,187,13,205,139,12,84,48,218,228,35,240,86,70,203,121,239,228,210,
	96,86,116,12,5,10,97,49,111,52,238,79,12,36,119,59,39,118,53,49,89,216,181,
	248,68,147,193,146,59,21,88,196,217,4,120,51,69,7,12,32,80,227,49,80,232,6,
	163,221,171,38,126,130,81,154,216,165,93,120,40,186,136,115,223,1,231,48,
	101,208,254,192,23,23,206,172,130,253,9,225,127,116,192,192,13,149,20,201,
	60,244,151,226,114,71,81,114,126,198,247,177,98,70,70,62,181,18,74,140,239,
	245,248,62,71,1,107,238,209,77,222,10,141,7,71,29,14,62,168,83,126,138,139,
	26,106
};

void TestMapBlock::testLoad29(IGameDef *gamedef)
{
	UASSERT(MAP_BLOCKSIZE == 16);
	const std::string_view buf(reinterpret_cast<const char*>(coded_mapblock29), sizeof(coded_mapblock29));

	// this node is not part of the test gamedef, so we also test handling of
	// unknown nodes here.
	auto *ndef = gamedef->getNodeDefManager();
	UASSERT(ndef->getId("default:chest") == CONTENT_IGNORE);

	std::istringstream iss;
	iss.str(std::string(buf));
	u8 version = readU8(iss);
	UASSERTEQ(int, version, 29);
	MapBlock block({}, gamedef);
	block.deSerialize(iss, version, true);

	auto content_chest = ndef->getId("default:chest");
	UASSERT(content_chest != CONTENT_IGNORE);

	// there are bricks at each edge pos
	const v3s16 pl[] = {
		{0, 0, 0}, {15, 0, 0}, {0, 15, 0}, {0, 0, 15},
		{15, 15, 0}, {0, 15, 15}, {15, 15, 15},
	};
	for (auto p : pl)
		UASSERTEQ(int, block.getNodeNoEx(p).getContent(), t_CONTENT_BRICK);

	// and a chest with an item inside
	UASSERTEQ(int, block.getNodeNoEx({0, 1, 0}).getContent(), content_chest);
	auto *meta = block.m_node_metadata.get({0, 1, 0});
	UASSERT(meta);
	auto *inv = meta->getInventory();
	UASSERT(inv);
	auto *ilist = inv->getList("main");
	UASSERT(ilist);
	UASSERTEQ(int, ilist->getSize(), 32);
	UASSERTEQ(auto, ilist->getItem(1).name, "default:stone");
}

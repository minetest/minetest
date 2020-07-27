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

#include <cstring>

#include "test.h"

#include "util/invertedindex.h"
#include "log.h"

class TestInvertedIndex : public TestBase {
public:
	TestInvertedIndex() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestInvertedIndex"; }

	void runTests(IGameDef *gamedef);

	void testInvertedIndex();

protected:
	static const std::vector<u32> cases[];
};

static TestInvertedIndex g_test_instance;

void TestInvertedIndex::runTests(IGameDef *gamedef)
{
	TEST(testInvertedIndex);
}

////////////////////////////////////////////////////////////////////////////////

void TestInvertedIndex::testInvertedIndex()
{
	InvertedIndex index;
	unsigned char hash1[SHA256_DIGEST_LENGTH];
	unsigned char hash2[SHA256_DIGEST_LENGTH];

	index.getHash(hash1);
	index.index(aabb3f(-.5f, -.5f, -.5f, .5f, .5f, .5f));
	index.getHash(hash2);
	UASSERT(std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	index.index(aabb3f(0.f, -.5f, .5f, 1.5f, 2.5f, .65f));
	index.getHash(hash1);
	UASSERT(std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	index.index(aabb3f(.3f, .5f, -.5f, .5f, 1.5f, .6f));
	index.getHash(hash2);
	UASSERT(std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	index.index(aabb3f(.0f, .0f, .0f, 2.5f, .5f, 1.5f));
	index.getHash(hash1);
	UASSERT(std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	index.index(aabb3f(17.f, -.5f, -3.5f, 18.f, .5f, -1.5f));
	index.getHash(hash2);
	UASSERT(std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	// MinX -.5 (0) 0. (1,3) .3 (2) 17. (4)
	// MinY -.5 (0,1,4) 0. (3) .5 (2)
	// MinZ -3.5 (4) -.5 (0,2) 0. (3) .5 (1) 
	// MaxX .5 (0,2) 1.5 (1) 2.5 (3) 18. (4)
	// MaxY .5 (0,3,4) 1.5 (2) 1.5 (1)
	// MaxZ -1.5 (4) .5 (0) .6 (2) .65 (1) 
	// MaxWidth 2.5 3. 2.
	UASSERT(index.getMaxWidth().equals(v3f(2.5f, 3.f, 2.f), .001f));
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -1.f), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.5f), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.4f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 1.f), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.f), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 4);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -1.f), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.5f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.4f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 1.f), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.f), 4);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 4);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -1.f), -1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.5f), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.4f), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 1.f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.f), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -1.f), -1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.5f), -1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.4f), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.f), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 1.f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.f), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 3);

	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -1.f, 1, 4), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.5f, 1, 4), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.4f, 1, 4), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.f, 1, 4), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 0, 3), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 0, 3), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 1.f, 0, 3), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.f, 0, 3), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 0, 3), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -1.f, 1, 4), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.5f, 1, 4), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.4f, 1, 4), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.f, 0, 3), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 0, 3), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 0, 3), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 1.f, 0, 3), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.f, 0, 3), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 0, 3), 3);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -1.f, 3, 0), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.5f, 3, 0), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.4f, 3, 0), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.f, 3, 0), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 3, 0), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 2, -1), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 1.f, 2, -1), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.f, 2, -1), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 2, -1), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -1.f, 3, 0), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.5f, 3, 0), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.4f, 3, 0), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.f, 3, 0), 0);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 3, 0), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 3, 0), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 1.f, 2, -1), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.f, 2, -1), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 2, -1), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	const AttributeIndex *ai = index.getAttributeIndex(COLLISION_BOX_MAX_Z, 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(f32, ai->first, 0.6f);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(u32, ai->second.size(), 1);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
	UASSERTEQ(u32, ai->second.at(0), 2);
	index.getHash(hash1);
	UASSERT(!std::memcmp(hash1, hash2, SHA256_DIGEST_LENGTH));
}

const std::vector<u32> TestInvertedIndex::cases[] = {
		std::vector<u32>{10, 20, 30, 40, 50, 60, 70, 80, 90, 100},
		std::vector<u32>{4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40},
		std::vector<u32>{6, 9, 12, 15, 18, 21, 24, 27, 30},
		std::vector<u32>{10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100},
		std::vector<u32>{14, 21, 28, 35, 42, 49, 56, 63, 70},
		std::vector<u32>{22, 33, 44, 55, 66, 77, 88, 99},
		std::vector<u32>{26, 39},
		std::vector<u32>{2, 3, 5, 7, 11, 13, 17, 19, 23},
		std::vector<u32>{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
	};

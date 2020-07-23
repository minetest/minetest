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

#include "util/invertedindex.h"
#include "log.h"

class TestInvertedIndex : public TestBase {
public:
	TestInvertedIndex() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestInvertedIndex"; }

	void runTests(IGameDef *gamedef);

	void testSingleIndexListIterator();
	void testIndexListIteratorSet_union();
	void testIndexListIteratorSet_union1();
	void testIndexListIteratorSet_union2();
	void testIndexListIteratorSet_intersection();
	void testIndexListIteratorSet_intersection1();
	void testIndexListIteratorSet_intersection2();
	void testIndexListIteratorDifference();
	void testInvertedIndex();

protected:
	static const std::vector<u32> cases[];
};

static TestInvertedIndex g_test_instance;

void TestInvertedIndex::runTests(IGameDef *gamedef)
{
	TEST(testSingleIndexListIterator);
	TEST(testIndexListIteratorSet_union);
	TEST(testIndexListIteratorSet_union1);
	TEST(testIndexListIteratorSet_union2);
	TEST(testIndexListIteratorSet_intersection);
	TEST(testIndexListIteratorSet_intersection1);
	TEST(testIndexListIteratorSet_intersection2);
	TEST(testIndexListIteratorDifference);
	TEST(testInvertedIndex);
}

////////////////////////////////////////////////////////////////////////////////

void TestInvertedIndex::testSingleIndexListIterator()
{
	SingleIndexListIterator iter(COLLISION_FACE_MIN_X, 3.0f, &cases[0]);
	f32 offset;
	UASSERT(iter.hasNext());
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter.peek(), 10);
	UASSERT(iter.forward());
	UASSERT(iter.hasNext());
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter.peek(), 20);
	UASSERT(iter.skipForward(30));
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter.peek(), 30);
	UASSERT(iter.skipForward(45));
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter.peek(), 50);
	UASSERT(iter.forward());
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter.peek(), 60);
	UASSERT(!iter.skipForward(200));
}

void TestInvertedIndex::testIndexListIteratorSet_union1()
{
	IndexListIteratorSet set;
	set.add(COLLISION_FACE_MIN_X, 3.0f, &cases[0]);
	IndexListIterator *iter = set.getUnion();
	f32 offset;
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 10);
	UASSERT(iter->forward());
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 20);
	UASSERT(iter->skipForward(30));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 30);
	UASSERT(iter->skipForward(45));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 50);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 60);
	UASSERT(!iter->skipForward(200));
}

void TestInvertedIndex::testIndexListIteratorSet_intersection1()
{
	IndexListIteratorSet set;
	set.add(COLLISION_FACE_MIN_X, 3.0f, &cases[0]);
	IndexListIterator *iter = set.getIntersection();
	f32 offset;
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 10);
	UASSERT(iter->forward());
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 20);
	UASSERT(iter->skipForward(30));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 30);
	UASSERT(iter->skipForward(45));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 50);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 60);
	UASSERT(!iter->skipForward(200));
}

void TestInvertedIndex::testIndexListIteratorSet_union2()
{
	IndexListIteratorSet set;
	set.add(COLLISION_FACE_MIN_X, 3.0f, &cases[1]); // twos
	set.add(COLLISION_FACE_MIN_Y, 2.0f, &cases[2]); // threes
	IndexListIterator *iter = set.getUnion(); // 4, 6, 8, 9, 10, 12, 14, 15, 16, 18, 20, 21, 22, 24, 26, 27, 28, 30, 32, 34, 36, 38, 40
	f32 offset;
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 4);
	UASSERT(iter->forward());
	UASSERT(iter->hasNext());
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 6);
	UASSERT(iter->skipForward(15));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_Y);
	UASSERTEQ(f32, offset, 2.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 15);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 16);
	UASSERT(iter->skipForward(31));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 32);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 34);
	UASSERT(iter->skipForward(40));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 40);
	UASSERT(!iter->forward());
}

void TestInvertedIndex::testIndexListIteratorSet_intersection2()
{
	IndexListIteratorSet set;
	set.add(COLLISION_FACE_MIN_X, 3.0f, &cases[1]); // twos
	set.add(COLLISION_FACE_MIN_X, 3.0f, &cases[2]); // threes
	IndexListIterator *iter = set.getIntersection(); // 6, 12, 18, 24, 30
	f32 offset;
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 6);
	UASSERT(iter->forward());
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 12);
	UASSERT(iter->skipForward(18));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 18);
	UASSERT(iter->skipForward(28));
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 3.0f);
	UASSERTEQ(u32, iter->peek(), 30);
	UASSERT(!iter->skipForward(200));
}

void TestInvertedIndex::testIndexListIteratorDifference()
{
	SingleIndexListIterator
			twos(COLLISION_FACE_MIN_Y, 1.f, &cases[1]),
			threes(COLLISION_FACE_MIN_Y, 1.f, &cases[2]);
	IndexListIteratorDifference iter(&twos, &threes); // 4, 8, 10, 14, 16, 20, 22, 26, 28, 32, 34, 36, 38, 40
	f32 offset;
	UASSERT(iter.hasNext());
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_Y);
	UASSERTEQ(f32, offset, 1.0f);
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter.peek(), 4);
	UASSERT(iter.forward());
	UASSERT(iter.hasNext());
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_Y);
	UASSERTEQ(f32, offset, 1.0f);
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter.peek(), 8);
	UASSERT(iter.forward());
	UASSERT(iter.hasNext());
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_Y);
	UASSERTEQ(f32, offset, 1.0f);
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter.peek(), 10);
	UASSERT(iter.skipForward(15));
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_MIN_Y);
	UASSERTEQ(f32, offset, 1.0f);
	UASSERTEQ(CollisionFace, iter.nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter.peek(), 16);
	UASSERT(iter.skipForward(24));
	UASSERTEQ(u32, iter.peek(), 26);
	UASSERT(iter.forward());
	UASSERTEQ(u32, iter.peek(), 28);
	UASSERT(iter.forward());
	UASSERTEQ(u32, iter.peek(), 32);
	UASSERT(iter.skipForward(37));
	UASSERTEQ(u32, iter.peek(), 38);
	UASSERT(iter.forward());
	UASSERTEQ(u32, iter.peek(), 40);
	UASSERT(!iter.forward());
}

void TestInvertedIndex::testIndexListIteratorSet_union()
{
	SingleIndexListIterator one(COLLISION_FACE_MIN_X, 1.0f, &cases[1]);
	SingleIndexListIterator two(COLLISION_FACE_MAX_Z, 4.0f, &cases[3]);
	IndexListIteratorSet set;
	set.add(&one); // twos MinX 1
	set.add(COLLISION_FACE_MAX_Y, -3.2f, &cases[2]);	// threes MaxY -3.2
	set.add(COLLISION_FACE_MAX_Y, -3.2f, &cases[4]);	// sevens MaxY -3.2
	set.add(&two);	// fives MaxZ 4.0
	set.add(COLLISION_FACE_MIN_X, 2.0f, &cases[5]);	// elevens MinX 2
	set.add(COLLISION_FACE_MIN_X, 1.0f, &cases[7]);	// primes MinX 1
	set.add(COLLISION_FACE_MIN_Z, 3.0f, &cases[6]);	// thirteens MinZ 3
	set.add(COLLISION_FACE_MIN_X, 1.5f, &cases[0]);	// tens MinX 1.5
	IndexListIterator *iter = set.getUnion();
	f32 offset;
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 1.f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 2);
	UASSERT(iter->forward());
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 1.f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 3);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 1.f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 4);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 1.f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 5);
	UASSERT(iter->forward());
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 6);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 1.f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 7);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MIN_X);
	UASSERTEQ(f32, offset, 1.f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 8);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_MAX_Y);
	UASSERTEQ(f32, offset, -3.2f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 9);
	UASSERT(iter->forward());
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 10);
	UASSERT(iter->skipForward(15));
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 15);
	UASSERT(iter->forward());
	UASSERTEQ(u32, iter->peek(), 16);
	UASSERT(iter->forward());
	UASSERTEQ(u32, iter->peek(), 17);
	UASSERT(iter->skipForward(29));
	UASSERTEQ(u32, iter->peek(), 30);
	UASSERT(iter->forward());
	UASSERTEQ(u32, iter->peek(), 32);
	UASSERT(iter->skipForward(67));
	UASSERTEQ(u32, iter->peek(), 70);
	UASSERT(iter->skipForward(95));
	UASSERTEQ(u32, iter->peek(), 95);
	UASSERT(iter->forward());
	UASSERTEQ(u32, iter->peek(), 99);
	UASSERT(iter->forward());
	UASSERTEQ(u32, iter->peek(), 100);
	UASSERT(!iter->forward());
	UASSERT(!iter->hasNext());
	delete iter;
}

void TestInvertedIndex::testIndexListIteratorSet_intersection()
{
	IndexListIteratorSet set;
	set.add(COLLISION_FACE_MAX_Y, -3.2f, &cases[0]);	// tens
	set.add(COLLISION_FACE_MAX_Z, .2f, &cases[1]);	// twos
	set.add(COLLISION_FACE_MIN_X, 2.0f, &cases[3]);	// fives
	IndexListIterator *iter = set.getIntersection();
	f32 offset;
	UASSERT(iter->hasNext());
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 10);
	UASSERT(iter->forward());
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 20);
	UASSERT(iter->skipForward(33));
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERT(iter->nextFace(&offset) != COLLISION_FACE_NONE);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 40);
	UASSERT(!iter->forward());
	UASSERT(!iter->hasNext());
	delete iter;
}

void TestInvertedIndex::testInvertedIndex()
{
	InvertedIndex index;

	index.index(aabb3f(-.5f, -.5f, -.5f, .5f, .5f, .5f));
	index.index(aabb3f(0.f, -.5f, .5f, 1.5f, 2.5f, .65f));
	index.index(aabb3f(.3f, .5f, -.5f, .5f, 1.5f, .6f));
	index.index(aabb3f(.0f, .0f, .0f, 2.5f, .5f, 1.5f));
	index.index(aabb3f(17.f, -.5f, -3.5f, 18.f, .5f, -1.5f));
	// MinX -.5 (0) 0. (1,3) .3 (2) 17. (4)
	// MinY -.5 (0,1,4) 0. (3) .5 (2)
	// MinZ -3.5 (4) -.5 (0,2) 0. (3) .5 (1) 
	// MaxX .5 (0,2) 1.5 (1) 2.5 (3) 18. (4)
	// MaxY .5 (0,3,4) 1.5 (2) 1.5 (1)
	// MaxZ -1.5 (4) .5 (0) .6 (2) .65 (1) 
	// MaxWidth 2.5 3. 2.
	UASSERT(index.getMaxWidth().equals(v3f(2.5f, 3.f, 2.f), .001f));
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -1.f), 0);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.5f), 0);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.4f), 1);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.f), 1);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 2);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 2);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 1.f), 3);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.f), 3);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 4);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -1.f), 0);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.5f), 1);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.4f), 1);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.f), 2);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 2);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 3);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 1.f), 3);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.f), 4);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 4);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -1.f), -1);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.5f), 0);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.4f), 0);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.f), 1);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 1);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 2);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 1.f), 2);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.f), 3);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 3);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -1.f), -1);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.5f), -1);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.4f), 0);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.f), 0);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f), 1);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f), 1);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 1.f), 2);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.f), 2);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f), 3);

	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -1.f, 1, 4), 1);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.5f, 1, 4), 1);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, -.4f, 1, 4), 1);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.f, 1, 4), 1);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 0, 3), 2);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 0, 3), 2);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 1.f, 0, 3), 3);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.f, 0, 3), 3);
	UASSERTEQ(s32, index.lowerAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 0, 3), 3);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -1.f, 1, 4), 1);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.5f, 1, 4), 1);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, -.4f, 1, 4), 1);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.f, 0, 3), 2);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 0, 3), 2);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 0, 3), 3);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 1.f, 0, 3), 3);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.f, 0, 3), 3);
	UASSERTEQ(s32, index.upperAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 0, 3), 3);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -1.f, 3, 0), 0);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.5f, 3, 0), 0);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, -.4f, 3, 0), 0);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.f, 3, 0), 1);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 3, 0), 1);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 2, -1), 2);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 1.f, 2, -1), 2);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.f, 2, -1), 2);
	UASSERTEQ(s32, index.lowerBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 2, -1), 2);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -1.f, 3, 0), 0);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.5f, 3, 0), 0);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, -.4f, 3, 0), 0);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.f, 3, 0), 0);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.1f, 3, 0), 1);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 0.3f, 3, 0), 1);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 1.f, 2, -1), 2);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.f, 2, -1), 2);
	UASSERTEQ(s32, index.upperBackAttributeBound(COLLISION_BOX_MIN_X, 17.1f, 2, -1), 2);
	const AttributeIndex *ai = index.getAttributeIndex(COLLISION_BOX_MAX_Z, 2);
	UASSERTEQ(f32, ai->first, 0.6f);
	UASSERTEQ(u32, ai->second.size(), 1);
	UASSERTEQ(u32, ai->second.at(0), 2);
	IndexListIteratorSet set;
	index.getInterval(COLLISION_BOX_MIN_Z, -.5f, 0.f, &set); // Should be empty
	IndexListIterator *iter = set.getUnion();
	UASSERT(!iter->hasNext());
	delete iter;
	index.getInterval(COLLISION_BOX_MIN_Z, 0.f, -.5f, &set); // Should be 0, 2, 3
	iter = set.getUnion();
	UASSERT(iter->hasNext());
	f32 offset;
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_BOX_MIN_Z);
	UASSERTEQ(f32, offset, -.5f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 0);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_BOX_MIN_Z);
	UASSERTEQ(f32, offset, -.5f);
	UASSERTEQ(u32, iter->peek(), 2);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_BOX_MIN_Z);
	UASSERTEQ(f32, offset, 0.f);
	UASSERTEQ(u32, iter->peek(), 3);
	UASSERT(!iter->forward());
	delete iter;
	index.getHalfOpenInterval(COLLISION_BOX_MIN_Z, 0.f, -.5f, &set); // Should be 3
	iter = set.getUnion();
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_BOX_MIN_Z);
	UASSERTEQ(f32, offset, 0.f);
	UASSERTEQ(u32, iter->peek(), 3);
	UASSERT(!iter->forward());
	delete iter;
	index.getHalfOpenInterval(COLLISION_BOX_MIN_Z, -.5f, 0.f, &set); // Should be 0, 2
	iter = set.getUnion();
	UASSERT(iter->hasNext());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_BOX_MIN_Z);
	UASSERTEQ(f32, offset, -.5f);
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_FACE_NONE);
	UASSERTEQ(u32, iter->peek(), 0);
	UASSERT(iter->forward());
	UASSERTEQ(CollisionFace, iter->nextFace(&offset), COLLISION_BOX_MIN_Z);
	UASSERTEQ(f32, offset, -.5f);
	UASSERTEQ(u32, iter->peek(), 2);
	UASSERT(!iter->forward());
	delete iter;
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

 /*
Minetest
Copyright (C) 2016-2018 Ben Deutsch <ben@bendeutsch.de>

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

#include "splinesequence.h"
#include "irrlichttypes_extrabloated.h"

class TestSplineSequence : public TestBase {
public:
	TestSplineSequence() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestSplineSequence"; }

	void runTests(IGameDef *gamedef);

	void testBasic();
	void testDegree0();
	void testNormalize();
	void testUnNormalized();

	void testDegree2();
	void testDegree3();

	void testV3f();
	void testQuaternion();
};

static TestSplineSequence g_test_instance;

void TestSplineSequence::runTests(IGameDef *gamedef)
{
	TEST(testBasic);
	TEST(testDegree0);
	TEST(testNormalize);
	TEST(testUnNormalized);
	TEST(testDegree2);
	TEST(testDegree3);
	TEST(testV3f);
	TEST(testQuaternion);
}

////////////////////////////////////////////////////////////////////////////////

void TestSplineSequence::testBasic()
{
	SplineSequence<double> spline_sequence;
	spline_sequence
		.addNode(0.0)
		.addNode(1.0)
		.addNode(3.0);
	spline_sequence.addIndex(1.0f, 0, 1);
	double result = 0.0;
	spline_sequence.interpolate(result, 0.5f);
	UASSERTNEAR(double, result, 0.5, 0.0001);
	UASSERTEQ(float, spline_sequence.getTotalDuration(), 1.0f);
}

// Should work for degree 0, too!
// (i.e. always return the first node)
//
void TestSplineSequence::testDegree0()
{
	SplineSequence<double> spline_sequence;
	spline_sequence
		.addNode(4.0)
		.addNode(5.0);
	spline_sequence.addIndex(1.0f, 0, 0);
	double result = 0.0;
	spline_sequence.interpolate(result, 0.5f);
	UASSERTEQ(double, result, 4.0);
}

void TestSplineSequence::testNormalize()
{
    SplineSequence<double> spline_sequence;
	spline_sequence
		.addNode(3.0)
		.addNode(4.0)
		.addNode(5.0)
		.addNode(6.0);
	spline_sequence.addIndex(1.0f, 0, 1);
	spline_sequence.addIndex(2.0f, 2, 1);
	UASSERTNEAR(float, spline_sequence.getTotalDuration(), 3.0f, 0.0001f);
    spline_sequence.normalizeDurations();
	UASSERTNEAR(float, spline_sequence.getTotalDuration(), 1.0f, 0.0001f);
	double result = 0.0;
	spline_sequence.interpolate(result, 0.33f);
	UASSERTNEAR(double, result, 4.0, 0.01);
	spline_sequence.interpolate(result, 0.34f);
	UASSERTNEAR(double, result, 5.0, 0.01);
}

// Un-normalized should also work, just remember the total!
// If this is not wanted, adjust this test
//
void TestSplineSequence::testUnNormalized()
{
    SplineSequence<double> spline_sequence;
	spline_sequence
		.addNode(3.0)
		.addNode(4.0)
		.addNode(5.0)
		.addNode(6.0);
	spline_sequence.addIndex(1.0f, 0, 1);
	spline_sequence.addIndex(2.0f, 2, 1);
	// not spline_sequence.normalizeDurations();
	UASSERTNEAR(float, spline_sequence.getTotalDuration(), 3.0f, 0.0001f);
	double result = 0.0;
	spline_sequence.interpolate(result, 0.99f);
	UASSERTNEAR(double, result, 4.0, 0.01);
	spline_sequence.interpolate(result, 1.01f);
	UASSERTNEAR(double, result, 5.0, 0.01);
}

void TestSplineSequence::testDegree2()
{
	SplineSequence<double> spline_sequence;
	spline_sequence
		.addNode(0.0)
		.addNode(1.0)
		.addNode(3.0);
	spline_sequence.addIndex(1.0f, 0, 2);
	double result = 0.0;

	// (0.5 + 2.0) / 2.0 = 1.25
	spline_sequence.interpolate(result, 0.5f);
	UASSERTNEAR(double, result, 1.25, 0.0001);
}

void TestSplineSequence::testDegree3()
{
	SplineSequence<double> spline_sequence;
	spline_sequence
		.addNode(0.0)
		.addNode(1.0)
		.addNode(3.0)
		.addNode(7.0);
	spline_sequence.addIndex(1.0f, 0, 3);
	double result = 0.0;

	// (0.5 + 2.0) / 2.0 = 
	spline_sequence.interpolate(result, 0.5f);
	double expect = (
		((0.0 + 1.0) * 0.5 +
		(1.0 + 3.0) * 0.5) * 0.5 +
		((1.0 + 3.0) * 0.5 +
		(3.0 + 7.0) * 0.5) * 0.5
	) * 0.5;
	UASSERTNEAR(double, result, expect, 0.0001);
}

void TestSplineSequence::testV3f()
{
	SplineSequence<v3f> spline_sequence;
	spline_sequence
		.addNode(v3f(0.0f, 1.0f, 2.0f))
		.addNode(v3f(1.0f, 0.0f, 2.0f))
		.addNode(v3f(0.0f, 0.0f, 1.0f));
	spline_sequence.addIndex(1.0f, 0, 2);
	v3f result(0.0f, 0.0f, 0.0f);

	// visually inspect values like this:
#if 0
	for(float a=0.1f; a<1.0f; a+=0.1f) {
		spline_sequence.interpolate(result, a);
		std::cout << result.X << ", " << result.Y << ", " << result.Z << std::endl;
	}
#endif

	// test two points: 0.2 and 0.6 - should be enough
	spline_sequence.interpolate(result, 0.2f);
	UASSERTNEAR(float, result.X, 0.32f, 0.001f);
	UASSERTNEAR(float, result.Y, 0.64f, 0.001f);
	UASSERTNEAR(float, result.Z, 1.96f, 0.001f);

	spline_sequence.interpolate(result, 0.6f);
	UASSERTNEAR(float, result.X, 0.48f, 0.001f);
	UASSERTNEAR(float, result.Y, 0.16f, 0.001f);
	UASSERTNEAR(float, result.Z, 1.64f, 0.001f);
}

// Support functions for the quaternion test
// We're usually going for rotations, so test with those,
// usually as Euler angles, in degrees
core::quaternion from_euler(float x, float y, float z) {
	v3f vec(x, y, z);
	vec *= core::DEGTORAD;
	return core::quaternion(vec);
}
v3f to_euler(core::quaternion q) {
	v3f vec;
	q.toEuler(vec);
	vec *= core::RADTODEG;
	return vec;
}

void TestSplineSequence::testQuaternion()
{
	SplineSequence<core::quaternion> spline_sequence;
	spline_sequence
		.addNode(from_euler(0.0f, 0.0f, -90.0f))
		.addNode(from_euler(90.0f, 0.0f, -90.0f))
		.addNode(from_euler(90.0f, 0.0f, 0.0f));
	spline_sequence.addIndex(1.0f, 0, 2);
	core::quaternion result;

	// visually inspect values like this:
#if 0
	for(float a=0.0f; a<=1.0f; a+=0.1f) {
		spline_sequence.interpolate(result, a);
		v3f result_euler = to_euler(result);
		std::cout << result_euler.X << ", " << result_euler.Y << ", " << result_euler.Z << std::endl;
	}
#endif

	// test two points: 0.2 and 0.6 - should be enough
	spline_sequence.interpolate(result, 0.2f);
	v3f result1 = to_euler(result);
	UASSERTNEAR(float, result1.X, 32.3f, 0.1f);
	UASSERTNEAR(float, result1.Y, 1.8f, 0.1f);
	UASSERTNEAR(float, result1.Z, -86.6f, 0.1f);

	spline_sequence.interpolate(result, 0.6f);
	v3f result2 = to_euler(result);
	UASSERTNEAR(float, result2.X, 75.7f, 0.1f);
	UASSERTNEAR(float, result2.Y, 4.1f, 0.1f);
	UASSERTNEAR(float, result2.Z, -57.5f, 0.1f);
}

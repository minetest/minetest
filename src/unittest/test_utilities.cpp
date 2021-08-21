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

#include "util/enriched_string.h"
#include "util/numeric.h"
#include "util/string.h"
#include "util/base64.h"
#include "util/time_parsing.h"
#include "util/Optional.h"
#include "util/thread.h"
#include <atomic>
#include <cmath>
#include <functional>
#include <limits>

class TestUtilities : public TestBase {
public:
	TestUtilities() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestUtilities"; }

	void runTests(IGameDef *gamedef);

	void testAngleWrapAround();
	void testWrapDegrees_0_360_v3f();
	void testLowercase();
	void testTrim();
	void testIsYes();
	void testRemoveStringEnd();
	void testUrlEncode();
	void testUrlDecode();
	void testPadString();
	void testStartsWith();
	void testStrEqual();
	void testStringTrim();
	void testStrToIntConversion();
	void testStringReplace();
	void testStringAllowed();
	void testAsciiPrintableHelper();
	void testUTF8();
	void testRemoveEscapes();
	void testWrapRows();
	void testEnrichedString();
	void testIsNumber();
	void testIsPowerOfTwo();
	void testMyround();
	void testStringJoin();
	void testEulerConversion();
	void testBase64();
	void testParseDifftime();
	void testClassMoveTester();
	void testOptional();
	void testLambdaThread();
};

static TestUtilities g_test_instance;

void TestUtilities::runTests(IGameDef *gamedef)
{
	TEST(testAngleWrapAround);
	TEST(testWrapDegrees_0_360_v3f);
	TEST(testLowercase);
	TEST(testTrim);
	TEST(testIsYes);
	TEST(testRemoveStringEnd);
	TEST(testUrlEncode);
	TEST(testUrlDecode);
	TEST(testPadString);
	TEST(testStartsWith);
	TEST(testStrEqual);
	TEST(testStringTrim);
	TEST(testStrToIntConversion);
	TEST(testStringReplace);
	TEST(testStringAllowed);
	TEST(testAsciiPrintableHelper);
	TEST(testUTF8);
	TEST(testRemoveEscapes);
	TEST(testWrapRows);
	TEST(testEnrichedString);
	TEST(testIsNumber);
	TEST(testIsPowerOfTwo);
	TEST(testMyround);
	TEST(testStringJoin);
	TEST(testEulerConversion);
	TEST(testBase64);
	TEST(testParseDifftime);
	TEST(testClassMoveTester);
	TEST(testOptional);
	TEST(testLambdaThread);
}

////////////////////////////////////////////////////////////////////////////////

inline float ref_WrapDegrees180(float f)
{
	// This is a slower alternative to the wrapDegrees_180() function;
	// used as a reference for testing
	float value = fmodf(f + 180, 360);
	if (value < 0)
		value += 360;
	return value - 180;
}


inline float ref_WrapDegrees_0_360(float f)
{
	// This is a slower alternative to the wrapDegrees_0_360() function;
	// used as a reference for testing
	float value = fmodf(f, 360);
	if (value < 0)
		value += 360;
	return value < 0 ? value + 360 : value;
}


void TestUtilities::testAngleWrapAround() {
    UASSERT(fabs(modulo360f(100.0) - 100.0) < 0.001);
    UASSERT(fabs(modulo360f(720.5) - 0.5) < 0.001);
    UASSERT(fabs(modulo360f(-0.5) - (-0.5)) < 0.001);
    UASSERT(fabs(modulo360f(-365.5) - (-5.5)) < 0.001);

    for (float f = -720; f <= -360; f += 0.25) {
        UASSERT(std::fabs(modulo360f(f) - modulo360f(f + 360)) < 0.001);
    }

    for (float f = -1440; f <= 1440; f += 0.25) {
        UASSERT(std::fabs(modulo360f(f) - fmodf(f, 360)) < 0.001);
        UASSERT(std::fabs(wrapDegrees_180(f) - ref_WrapDegrees180(f)) < 0.001);
        UASSERT(std::fabs(wrapDegrees_0_360(f) - ref_WrapDegrees_0_360(f)) < 0.001);
        UASSERT(wrapDegrees_0_360(
                std::fabs(wrapDegrees_180(f) - wrapDegrees_0_360(f))) < 0.001);
    }

}

void TestUtilities::testWrapDegrees_0_360_v3f()
{
    // only x test with little step
	for (float x = -720.f; x <= 720; x += 0.05) {
        v3f r = wrapDegrees_0_360_v3f(v3f(x, 0, 0));
        UASSERT(r.X >= 0.0f && r.X < 360.0f)
        UASSERT(r.Y == 0.0f)
        UASSERT(r.Z == 0.0f)
    }

    // only y test with little step
    for (float y = -720.f; y <= 720; y += 0.05) {
        v3f r = wrapDegrees_0_360_v3f(v3f(0, y, 0));
        UASSERT(r.X == 0.0f)
        UASSERT(r.Y >= 0.0f && r.Y < 360.0f)
        UASSERT(r.Z == 0.0f)
    }

    // only z test with little step
    for (float z = -720.f; z <= 720; z += 0.05) {
        v3f r = wrapDegrees_0_360_v3f(v3f(0, 0, z));
        UASSERT(r.X == 0.0f)
        UASSERT(r.Y == 0.0f)
        UASSERT(r.Z >= 0.0f && r.Z < 360.0f)
	}

    // test the whole coordinate translation
    for (float x = -720.f; x <= 720; x += 2.5) {
        for (float y = -720.f; y <= 720; y += 2.5) {
            for (float z = -720.f; z <= 720; z += 2.5) {
                v3f r = wrapDegrees_0_360_v3f(v3f(x, y, z));
                UASSERT(r.X >= 0.0f && r.X < 360.0f)
                UASSERT(r.Y >= 0.0f && r.Y < 360.0f)
                UASSERT(r.Z >= 0.0f && r.Z < 360.0f)
            }
        }
    }
}


void TestUtilities::testLowercase()
{
	UASSERT(lowercase("Foo bAR") == "foo bar");
	UASSERT(lowercase("eeeeeeaaaaaaaaaaaààààà") == "eeeeeeaaaaaaaaaaaààààà");
	UASSERT(lowercase("MINETEST-powa") == "minetest-powa");
}


void TestUtilities::testTrim()
{
	UASSERT(trim("") == "");
	UASSERT(trim("dirt_with_grass") == "dirt_with_grass");
	UASSERT(trim("\n \t\r  Foo bAR  \r\n\t\t  ") == "Foo bAR");
	UASSERT(trim("\n \t\r    \r\n\t\t  ") == "");
}


void TestUtilities::testIsYes()
{
	UASSERT(is_yes("YeS") == true);
	UASSERT(is_yes("") == false);
	UASSERT(is_yes("FAlse") == false);
	UASSERT(is_yes("-1") == true);
	UASSERT(is_yes("0") == false);
	UASSERT(is_yes("1") == true);
	UASSERT(is_yes("2") == true);
}


void TestUtilities::testRemoveStringEnd()
{
	const char *ends[] = {"abc", "c", "bc", "", NULL};
	UASSERT(removeStringEnd("abc", ends) == "");
	UASSERT(removeStringEnd("bc", ends) == "b");
	UASSERT(removeStringEnd("12c", ends) == "12");
	UASSERT(removeStringEnd("foo", ends) == "");
}


void TestUtilities::testUrlEncode()
{
	UASSERT(urlencode("\"Aardvarks lurk, OK?\"")
			== "%22Aardvarks%20lurk%2C%20OK%3F%22");
}


void TestUtilities::testUrlDecode()
{
	UASSERT(urldecode("%22Aardvarks%20lurk%2C%20OK%3F%22")
			== "\"Aardvarks lurk, OK?\"");
}


void TestUtilities::testPadString()
{
	UASSERT(padStringRight("hello", 8) == "hello   ");
}

void TestUtilities::testStartsWith()
{
	UASSERT(str_starts_with(std::string(), std::string()) == true);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string()) == true);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string("the")) == true);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string("The")) == false);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string("The"), true) == true);
	UASSERT(str_starts_with(std::string("T"), std::string("The")) == false);
}

void TestUtilities::testStrEqual()
{
	UASSERT(str_equal(utf8_to_wide("abc"), utf8_to_wide("abc")));
	UASSERT(str_equal(utf8_to_wide("ABC"), utf8_to_wide("abc"), true));
}


void TestUtilities::testStringTrim()
{
	UASSERT(trim("  a") == "a");
	UASSERT(trim("   a  ") == "a");
	UASSERT(trim("a   ") == "a");
	UASSERT(trim("") == "");
}


void TestUtilities::testStrToIntConversion()
{
	UASSERT(mystoi("123", 0, 1000) == 123);
	UASSERT(mystoi("123", 0, 10) == 10);
}


void TestUtilities::testStringReplace()
{
	std::string test_str;
	test_str = "Hello there";
	str_replace(test_str, "there", "world");
	UASSERT(test_str == "Hello world");
	test_str = "ThisAisAaAtest";
	str_replace(test_str, 'A', ' ');
	UASSERT(test_str == "This is a test");
}


void TestUtilities::testStringAllowed()
{
	UASSERT(string_allowed("hello", "abcdefghijklmno") == true);
	UASSERT(string_allowed("123", "abcdefghijklmno") == false);
	UASSERT(string_allowed_blacklist("hello", "123") == true);
	UASSERT(string_allowed_blacklist("hello123", "123") == false);
}

void TestUtilities::testAsciiPrintableHelper()
{
	UASSERT(IS_ASCII_PRINTABLE_CHAR('e') == true);
	UASSERT(IS_ASCII_PRINTABLE_CHAR('\0') == false);

	// Ensures that there is no cutting off going on...
	// If there were, 331 would be cut to 75 in this example
	// and 73 is a valid ASCII char.
	int ch = 331;
	UASSERT(IS_ASCII_PRINTABLE_CHAR(ch) == false);
}

void TestUtilities::testUTF8()
{
	UASSERT(utf8_to_wide("¤") == L"¤");

	UASSERT(wide_to_utf8(L"¤") == "¤");

	UASSERTEQ(std::string, wide_to_utf8(utf8_to_wide("")), "");
	UASSERTEQ(std::string, wide_to_utf8(utf8_to_wide("the shovel dug a crumbly node!")),
		"the shovel dug a crumbly node!");
	UASSERTEQ(std::string, wide_to_utf8(utf8_to_wide("-ä-")),
		"-ä-");
	UASSERTEQ(std::string, wide_to_utf8(utf8_to_wide("-\xF0\xA0\x80\x8B-")),
		"-\xF0\xA0\x80\x8B-");

}

void TestUtilities::testRemoveEscapes()
{
	UASSERT(unescape_enriched<wchar_t>(
		L"abc\x1bXdef") == L"abcdef");
	UASSERT(unescape_enriched<wchar_t>(
		L"abc\x1b(escaped)def") == L"abcdef");
	UASSERT(unescape_enriched<wchar_t>(
		L"abc\x1b((escaped with parenthesis\\))def") == L"abcdef");
	UASSERT(unescape_enriched<wchar_t>(
		L"abc\x1b(incomplete") == L"abc");
	UASSERT(unescape_enriched<wchar_t>(
		L"escape at the end\x1b") == L"escape at the end");
	// Nested escapes not supported
	UASSERT(unescape_enriched<wchar_t>(
		L"abc\x1b(outer \x1b(inner escape)escape)def") == L"abcescape)def");
}

void TestUtilities::testWrapRows()
{
	UASSERT(wrap_rows("12345678",4) == "1234\n5678");
	// test that wrap_rows doesn't wrap inside multibyte sequences
	{
		const unsigned char s[] = {
			0x2f, 0x68, 0x6f, 0x6d, 0x65, 0x2f, 0x72, 0x61, 0x70, 0x74, 0x6f,
			0x72, 0x2f, 0xd1, 0x82, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0x2f,
			0x6d, 0x69, 0x6e, 0x65, 0x74, 0x65, 0x73, 0x74, 0x2f, 0x62, 0x69,
			0x6e, 0x2f, 0x2e, 0x2e, 0};
		std::string str((char *)s);
		UASSERT(utf8_to_wide(wrap_rows(str, 20)) != L"<invalid UTF-8 string>");
	};
	{
		const unsigned char s[] = {
			0x74, 0x65, 0x73, 0x74, 0x20, 0xd1, 0x82, 0xd0, 0xb5, 0xd1, 0x81,
			0xd1, 0x82, 0x20, 0xd1, 0x82, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82,
			0x20, 0xd1, 0x82, 0xd0, 0xb5, 0xd1, 0x81, 0xd1, 0x82, 0};
		std::string str((char *)s);
		UASSERT(utf8_to_wide(wrap_rows(str, 8)) != L"<invalid UTF-8 string>");
	}
}

void TestUtilities::testEnrichedString()
{
	EnrichedString str(L"Test bar");
	irr::video::SColor color(0xFF, 0, 0, 0xFF);

	UASSERT(str.substr(1, 3).getString() == L"est");
	str += L" BUZZ";
	UASSERT(str.substr(9, std::string::npos).getString() == L"BUZZ");
	str.setDefaultColor(color); // Blue foreground
	UASSERT(str.getColors()[5] == color);
	// Green background, then white and yellow text
	str = L"\x1b(b@#0F0)Regular \x1b(c@#FF0)yellow";
	UASSERT(str.getColors()[2] == 0xFFFFFFFF);
	str.setDefaultColor(color); // Blue foreground
	UASSERT(str.getColors()[13] == 0xFFFFFF00); // Still yellow text
	UASSERT(str.getBackground() == 0xFF00FF00); // Green background
}

void TestUtilities::testIsNumber()
{
	UASSERT(is_number("123") == true);
	UASSERT(is_number("") == false);
	UASSERT(is_number("123a") == false);
}


void TestUtilities::testIsPowerOfTwo()
{
	UASSERT(is_power_of_two(0) == false);
	UASSERT(is_power_of_two(1) == true);
	UASSERT(is_power_of_two(2) == true);
	UASSERT(is_power_of_two(3) == false);
	for (int exponent = 2; exponent <= 31; ++exponent) {
		UASSERT(is_power_of_two((1 << exponent) - 1) == false);
		UASSERT(is_power_of_two((1 << exponent)) == true);
		UASSERT(is_power_of_two((1 << exponent) + 1) == false);
	}
	UASSERT(is_power_of_two(U32_MAX) == false);
}

void TestUtilities::testMyround()
{
	UASSERT(myround(4.6f) == 5);
	UASSERT(myround(1.2f) == 1);
	UASSERT(myround(-3.1f) == -3);
	UASSERT(myround(-6.5f) == -7);
}

void TestUtilities::testStringJoin()
{
	std::vector<std::string> input;
	UASSERT(str_join(input, ",") == "");

	input.emplace_back("one");
	UASSERT(str_join(input, ",") == "one");

	input.emplace_back("two");
	UASSERT(str_join(input, ",") == "one,two");

	input.emplace_back("three");
	UASSERT(str_join(input, ",") == "one,two,three");

	input[1] = "";
	UASSERT(str_join(input, ",") == "one,,three");

	input[1] = "two";
	UASSERT(str_join(input, " and ") == "one and two and three");
}


static bool within(const f32 value1, const f32 value2, const f32 precision)
{
	return std::fabs(value1 - value2) <= precision;
}

static bool within(const v3f &v1, const v3f &v2, const f32 precision)
{
	return within(v1.X, v2.X, precision) && within(v1.Y, v2.Y, precision)
		&& within(v1.Z, v2.Z, precision);
}

static bool within(const core::matrix4 &m1, const core::matrix4 &m2,
		const f32 precision)
{
	const f32 *M1 = m1.pointer();
	const f32 *M2 = m2.pointer();
	for (int i = 0; i < 16; i++)
		if (! within(M1[i], M2[i], precision))
			return false;
	return true;
}

static bool roundTripsDeg(const v3f &v, const f32 precision)
{
	core::matrix4 m;
	setPitchYawRoll(m, v);
	return within(v, getPitchYawRoll(m), precision);
}

void TestUtilities::testEulerConversion()
{
	// This test may fail on non-IEEE systems.
	// Low tolerance is 4 ulp(1.0) for binary floats with 24 bit mantissa.
	// (ulp = unit in the last place; ulp(1.0) = 2^-23).
	const f32 tolL = 4.76837158203125e-7f;
	// High tolerance is 2 ulp(180.0), needed for numbers in degrees.
	// ulp(180.0) = 2^-16
	const f32 tolH = 3.0517578125e-5f;
	v3f v1, v2;
	core::matrix4 m1, m2;
	const f32 *M1 = m1.pointer();
	const f32 *M2 = m2.pointer();

	// Check that the radians version and the degrees version
	// produce the same results. Check also that the conversion
	// works both ways for these values.
	v1 = v3f(M_PI/3.0, M_PI/5.0, M_PI/4.0);
	v2 = v3f(60.0f, 36.0f, 45.0f);
	setPitchYawRollRad(m1, v1);
	setPitchYawRoll(m2, v2);
	UASSERT(within(m1, m2, tolL));
	UASSERT(within(getPitchYawRollRad(m1), v1, tolL));
	UASSERT(within(getPitchYawRoll(m2), v2, tolH));

	// Check the rotation matrix produced.
	UASSERT(within(M1[0], 0.932004869f, tolL));
	UASSERT(within(M1[1], 0.353553385f, tolL));
	UASSERT(within(M1[2], 0.0797927827f, tolL));
	UASSERT(within(M1[4], -0.21211791f, tolL));
	UASSERT(within(M1[5], 0.353553355f, tolL));
	UASSERT(within(M1[6], 0.911046684f, tolL));
	UASSERT(within(M1[8], 0.293892622f, tolL));
	UASSERT(within(M1[9], -0.866025448f, tolL));
	UASSERT(within(M1[10], 0.404508471f, tolL));

	// Check that the matrix is still homogeneous with no translation
	UASSERT(M1[3] == 0.0f);
	UASSERT(M1[7] == 0.0f);
	UASSERT(M1[11] == 0.0f);
	UASSERT(M1[12] == 0.0f);
	UASSERT(M1[13] == 0.0f);
	UASSERT(M1[14] == 0.0f);
	UASSERT(M1[15] == 1.0f);
	UASSERT(M2[3] == 0.0f);
	UASSERT(M2[7] == 0.0f);
	UASSERT(M2[11] == 0.0f);
	UASSERT(M2[12] == 0.0f);
	UASSERT(M2[13] == 0.0f);
	UASSERT(M2[14] == 0.0f);
	UASSERT(M2[15] == 1.0f);

	// Compare to Irrlicht's results. To be comparable, the
	// angles must come in a different order and the matrix
	// elements to compare are different too.
	m2.setRotationRadians(v3f(v1.Z, v1.X, v1.Y));
	UASSERT(within(M1[0], M2[5], tolL));
	UASSERT(within(M1[1], M2[6], tolL));
	UASSERT(within(M1[2], M2[4], tolL));

	UASSERT(within(M1[4], M2[9], tolL));
	UASSERT(within(M1[5], M2[10], tolL));
	UASSERT(within(M1[6], M2[8], tolL));

	UASSERT(within(M1[8], M2[1], tolL));
	UASSERT(within(M1[9], M2[2], tolL));
	UASSERT(within(M1[10], M2[0], tolL));

	// Check that Eulers that produce near gimbal-lock still round-trip
	UASSERT(roundTripsDeg(v3f(89.9999f, 17.f, 0.f), tolH));
	UASSERT(roundTripsDeg(v3f(89.9999f, 0.f, 19.f), tolH));
	UASSERT(roundTripsDeg(v3f(89.9999f, 17.f, 19.f), tolH));

	// Check that Eulers at an angle > 90 degrees may not round-trip...
	v1 = v3f(90.00001f, 1.f, 1.f);
	setPitchYawRoll(m1, v1);
	v2 = getPitchYawRoll(m1);
	//UASSERT(within(v1, v2, tolL)); // this is typically false
	// ... however the rotation matrix is the same for both
	setPitchYawRoll(m2, v2);
	UASSERT(within(m1, m2, tolL));
}

void TestUtilities::testBase64()
{
	// Test character set
	UASSERT(base64_is_valid("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/") == true);
	UASSERT(base64_is_valid("/+9876543210"
		"zyxwvutsrqponmlkjihgfedcba"
		"ZYXWVUTSRQPONMLKJIHGFEDCBA") == true);
	UASSERT(base64_is_valid("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+.") == false);
	UASSERT(base64_is_valid("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789 /") == false);

	// Test empty string
	UASSERT(base64_is_valid("") == true);

	// Test different lengths, with and without padding,
	// with correct and incorrect padding
	UASSERT(base64_is_valid("A") == false);
	UASSERT(base64_is_valid("AA") == true);
	UASSERT(base64_is_valid("AAA") == true);
	UASSERT(base64_is_valid("AAAA") == true);
	UASSERT(base64_is_valid("AAAAA") == false);
	UASSERT(base64_is_valid("AAAAAA") == true);
	UASSERT(base64_is_valid("AAAAAAA") == true);
	UASSERT(base64_is_valid("AAAAAAAA") == true);
	UASSERT(base64_is_valid("A===") == false);
	UASSERT(base64_is_valid("AA==") == true);
	UASSERT(base64_is_valid("AAA=") == true);
	UASSERT(base64_is_valid("AAAA") == true);
	UASSERT(base64_is_valid("AAAA====") == false);
	UASSERT(base64_is_valid("AAAAA===") == false);
	UASSERT(base64_is_valid("AAAAAA==") == true);
	UASSERT(base64_is_valid("AAAAAAA=") == true);
	UASSERT(base64_is_valid("AAAAAAA==") == false);
	UASSERT(base64_is_valid("AAAAAAA===") == false);
	UASSERT(base64_is_valid("AAAAAAA====") == false);
	UASSERT(base64_is_valid("AAAAAAAA") == true);
	UASSERT(base64_is_valid("AAAAAAAA=") == false);
	UASSERT(base64_is_valid("AAAAAAAA==") == false);
	UASSERT(base64_is_valid("AAAAAAAA===") == false);
	UASSERT(base64_is_valid("AAAAAAAA====") == false);

	// Test if canonical encoding
	// Last character limitations, length % 4 == 3
	UASSERT(base64_is_valid("AAB") == false);
	UASSERT(base64_is_valid("AAE") == true);
	UASSERT(base64_is_valid("AAQ") == true);
	UASSERT(base64_is_valid("AAB=") == false);
	UASSERT(base64_is_valid("AAE=") == true);
	UASSERT(base64_is_valid("AAQ=") == true);
	UASSERT(base64_is_valid("AAAAAAB=") == false);
	UASSERT(base64_is_valid("AAAAAAE=") == true);
	UASSERT(base64_is_valid("AAAAAAQ=") == true);
	// Last character limitations, length % 4 == 2
	UASSERT(base64_is_valid("AB") == false);
	UASSERT(base64_is_valid("AE") == false);
	UASSERT(base64_is_valid("AQ") == true);
	UASSERT(base64_is_valid("AB==") == false);
	UASSERT(base64_is_valid("AE==") == false);
	UASSERT(base64_is_valid("AQ==") == true);
	UASSERT(base64_is_valid("AAAAAB==") == false);
	UASSERT(base64_is_valid("AAAAAE==") == false);
	UASSERT(base64_is_valid("AAAAAQ==") == true);

	// Extraneous character present
	UASSERT(base64_is_valid(".") == false);
	UASSERT(base64_is_valid("A.") == false);
	UASSERT(base64_is_valid("AA.") == false);
	UASSERT(base64_is_valid("AAA.") == false);
	UASSERT(base64_is_valid("AAAA.") == false);
	UASSERT(base64_is_valid("AAAAA.") == false);
	UASSERT(base64_is_valid("A.A") == false);
	UASSERT(base64_is_valid("AA.A") == false);
	UASSERT(base64_is_valid("AAA.A") == false);
	UASSERT(base64_is_valid("AAAA.A") == false);
	UASSERT(base64_is_valid("AAAAA.A") == false);
	UASSERT(base64_is_valid("\xE1""AAA") == false);

	// Padding in wrong position
	UASSERT(base64_is_valid("A=A") == false);
	UASSERT(base64_is_valid("AA=A") == false);
	UASSERT(base64_is_valid("AAA=A") == false);
	UASSERT(base64_is_valid("AAAA=A") == false);
	UASSERT(base64_is_valid("AAAAA=A") == false);
}

void TestUtilities::testParseDifftime()
{
#define TEST_CORRECT(str, expected_seconds)               \
	do {                                                  \
		auto t = parse_difftime(str);                     \
		UASSERT(t.has_value());                           \
		UASSERTEQ(auto, t.value(), (expected_seconds));   \
	} while (false)

#define TEST_INVALID(str)                                 \
	do {                                                  \
		auto t = parse_difftime(str);                     \
		UASSERT(!t.has_value());                          \
	} while (false)

	TEST_CORRECT("42 d", 3628800.0);
	TEST_CORRECT("5Ms ", 5000000.0);
	TEST_CORRECT("-10a", -315569520.0);
	TEST_CORRECT(" +0x3.0p13Kih", 90596966400.0);
	TEST_CORRECT("1ks", 1000.0);
	TEST_CORRECT("1Ts", 1000000000000.0);
	TEST_CORRECT("1 s", 1.0);
	TEST_CORRECT("1 min", 60.0);
	TEST_CORRECT("1 h", 3600.0);
	TEST_CORRECT("1 d", 86400.0);
	TEST_CORRECT("1 a", 31556952.0);
	TEST_CORRECT("1 ca", 315569.52);
	TEST_CORRECT("1 ma", 31556.952);
	TEST_CORRECT("1 ua", 31.556952);
	TEST_CORRECT("100 cs", 1.0);
	TEST_CORRECT("0s", 0.0);
	TEST_CORRECT("0", 0.0);
	// FIXME: this doesn't work because of -ffast-math
	//~ TEST_CORRECT("inf", std::numeric_limits<double>::infinity());
	TEST_CORRECT("infs", std::numeric_limits<double>::infinity());

	TEST_INVALID("");
	TEST_INVALID(" ");
	TEST_INVALID(" 4 2");
	TEST_INVALID("3 M s");
	TEST_INVALID("h3");
	TEST_INVALID("4kmph");
	TEST_INVALID("4km/h");
	TEST_INVALID("1 day");
	TEST_INVALID("11:00");
	TEST_INVALID("1 y");
	TEST_INVALID("1kis");

#undef TEST_INVALID
#undef TEST_CORRECT
}

struct ClassMoveTester
{
	ClassMoveTester() = default;

	ClassMoveTester(int val, int &copy_counter, int &move_counter) :
			m_val(val), m_copy_counter(copy_counter), m_move_counter(move_counter) {}

	ClassMoveTester(const ClassMoveTester &other) :
			m_val(other.m_val), m_copy_counter(other.m_copy_counter),
			m_move_counter(other.m_move_counter) { m_copy_counter += 1; }

	ClassMoveTester(ClassMoveTester &&other) :
			m_val(other.m_val), m_copy_counter(other.m_copy_counter),
			m_move_counter(other.m_move_counter) { other.m_val = -1;  m_move_counter += 1; }

	ClassMoveTester &operator=(const ClassMoveTester &other)
	{
		m_val = other.m_val;
		m_copy_counter += 1;
		return *this;
	}

	ClassMoveTester &operator=(ClassMoveTester &&other)
	{
		if (&other != this) {
			m_val = other.m_val;
			other.m_val = -1;
		}
		m_move_counter += 1;
		return *this;
	}

	int m_val = 0; // -1 means moved from
	int &m_copy_counter = s_fb_copy_counter;
	int &m_move_counter = s_fb_move_counter;

	static int s_fb_copy_counter;
	static int s_fb_move_counter;
};

int ClassMoveTester::s_fb_copy_counter = 0;
int ClassMoveTester::s_fb_move_counter = 0;

void TestUtilities::testClassMoveTester()
{
	int copy_counter = 0;
	int move_counter = 0;
	ClassMoveTester tstr1(1, copy_counter, move_counter);
	ClassMoveTester tstr2 = tstr1;
	UASSERTEQ(int, copy_counter, 1);
	UASSERTEQ(int, move_counter, 0);
	tstr1 = std::move(tstr2);
	UASSERTEQ(int, copy_counter, 1);
	UASSERTEQ(int, move_counter, 1);
	UASSERTEQ(int, tstr1.m_val, 1);
	UASSERTEQ(int, tstr2.m_val, -1);
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 0);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 0);

	ClassMoveTester::s_fb_copy_counter = 0;
	ClassMoveTester::s_fb_move_counter = 0;
}

void TestUtilities::testOptional()
{
	int copy_counter = 0;
	int move_counter = 0;
	ClassMoveTester::s_fb_copy_counter = 0;
	ClassMoveTester::s_fb_move_counter = 0;

	// constructors
	Optional<ClassMoveTester> o1 = nullopt; // uses the static counters
	UASSERTEQ(int, move_counter, 0);
	Optional<ClassMoveTester> o2 = ClassMoveTester(2, copy_counter, move_counter);
	UASSERTEQ(int, move_counter, 1);
	UASSERTEQ(int, copy_counter, 0);
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 0);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 0);

	// assignment-operators
	o1 = ClassMoveTester(1, copy_counter, move_counter); // impl ctor + mov-ass => 2 moves
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 0);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 1); // 1 move here
	UASSERTEQ(int, move_counter, 2); // and 1 here
	o2 = nullopt;
	UASSERTEQ(int, copy_counter, 0);
	UASSERTEQ(int, move_counter, 2);
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 0);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 1);

	// has_value, operator bool
	UASSERT(o1.has_value());
	UASSERT(!o2.has_value());
	UASSERT((bool)o1.has_value());
	UASSERT(!(bool)o2.has_value());
	UASSERTEQ(int, copy_counter, 0);
	UASSERTEQ(int, move_counter, 2);
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 0);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 1);

	// value
	const Optional<ClassMoveTester> &co1 = o1;
	UASSERTEQ(int, co1.value().m_val, 1); // const &
	o1.value().m_val = 11; // &
	UASSERTEQ(int, o1.value().m_val, 11);
	UASSERTEQ(int, copy_counter, 0);
	ClassMoveTester to1c = o1.value();
	UASSERTEQ(int, to1c.m_val, 11);
	UASSERTEQ(int, o1.value().m_val, 11);
	UASSERTEQ(int, move_counter, 2);
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 1);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 1);
	UASSERTEQ(int, copy_counter, 0);
	ClassMoveTester to1m = std::move(o1).value(); // &&
	UASSERTEQ(int, move_counter, 2);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 2);
	UASSERTEQ(int, copy_counter, 0);
	UASSERTEQ(int, o1.value().m_val, -1);
	UASSERT(o1.has_value()); // value was moved, not the Optional
	UASSERTEQ(int, to1m.m_val, 11);
	o1 = ClassMoveTester(3, copy_counter, move_counter);
	o2 = ClassMoveTester(4, copy_counter, move_counter);
	UASSERTEQ(int, move_counter, 5); // +3 moves to this (2 * impl ctor + mov-ass)
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 3); // +1 move here (mov-ass)
	ClassMoveTester to2m = std::move(o2.value()); // &, then move
	UASSERTEQ(int, move_counter, 6);
	UASSERTEQ(int, copy_counter, 0);
	UASSERTEQ(int, o2.value().m_val, -1);
	UASSERTEQ(int, to2m.m_val, 4);

	// value_or
	UASSERTEQ(int, copy_counter, 0);
	UASSERTEQ(int, move_counter, 6);
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 1);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 3);
	ClassMoveTester tdef(637, copy_counter, move_counter);
	Optional<ClassMoveTester> o0 = nullopt;
	o2 = ClassMoveTester(5, copy_counter, move_counter);
	UASSERTEQ(int, move_counter, 8); // +2 moves (impl ctor + mov-ass)
	UASSERTEQ(int, o0.value_or(tdef).m_val, 637);
	UASSERTEQ(int, copy_counter, 1); // tdef was copied
	UASSERTEQ(int, std::move(o0).value_or(tdef).m_val, 637);
	UASSERTEQ(int, copy_counter, 2); // tdef was copied
	UASSERTEQ(int, std::move(o2).value_or(tdef).m_val, 5);
	UASSERTEQ(int, move_counter, 9); // o2.m_val was moved
	UASSERTEQ(int, o0.value_or(std::move(tdef)).m_val, 637);
	UASSERTEQ(int, move_counter, 10); // tdef was moved

	UASSERTEQ(int, copy_counter, 2);
	UASSERTEQ(int, move_counter, 10);
	UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 1);
	UASSERTEQ(int, ClassMoveTester::s_fb_move_counter, 3);

	ClassMoveTester::s_fb_copy_counter = 0;
	ClassMoveTester::s_fb_move_counter = 0;
}

void TestUtilities::testLambdaThread()
{
	// constructors
	{
		std::atomic_int c(0);
		std::unique_ptr<Thread> t1 = make_Thread("bla", [&c] { c += 1; });
		std::unique_ptr<Thread> t2 = make_Thread([&c] { c += 1; });
		UASSERT(t1->start());
		UASSERT(t2->start());
		UASSERT(t1->wait());
		UASSERT(t2->wait());
		UASSERTEQ(int, c, 2);
	}

	// the example to move into the lambda
	{
		int copy_counter = 0;
		int move_counter = 0;
		ClassMoveTester::s_fb_copy_counter = 0;
		ClassMoveTester::s_fb_move_counter = 0;
		ClassMoveTester cmt(1, copy_counter, move_counter);

		auto t = make_Thread(std::bind([](ClassMoveTester &cmt) {
			(void)cmt;
		}, std::move(cmt)));
		UASSERTEQ(int, copy_counter, 0);
		UASSERTEQ(int, move_counter, 2);
		UASSERTEQ(int, cmt.m_val, -1);
		UASSERT(t->start());
		UASSERT(t->wait());
		UASSERTEQ(int, copy_counter, 0);
		UASSERTEQ(int, move_counter, 2);
		UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 0);
		UASSERTEQ(int, ClassMoveTester::s_fb_copy_counter, 0);
	}
}

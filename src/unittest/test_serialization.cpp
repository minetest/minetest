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

#include "util/string.h"
#include "util/serialize.h"

class TestSerialization : public TestBase {
public:
	TestSerialization() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestSerialization"; }

	void runTests(IGameDef *gamedef);
	void buildTestStrings();

	void testSerializeString();
	void testSerializeWideString();
	void testSerializeLongString();
	void testSerializeJsonString();
	void testSerializeHex();
	void testDeSerializeString();
	void testDeSerializeWideString();
	void testDeSerializeLongString();
	void testStreamRead();
	void testStreamWrite();

	std::string teststring2;
	std::wstring teststring2_w;
	std::string teststring2_w_encoded;

	static const u8 test_serialized_data[12 * 13];
};

static TestSerialization g_test_instance;

void TestSerialization::runTests(IGameDef *gamedef)
{
	buildTestStrings();

	TEST(testSerializeString);
	TEST(testDeSerializeString);
	TEST(testSerializeWideString);
	TEST(testDeSerializeWideString);
	TEST(testSerializeLongString);
	TEST(testDeSerializeLongString);
	TEST(testSerializeJsonString);
	TEST(testSerializeHex);
	TEST(testStreamRead);
	TEST(testStreamWrite);
}

////////////////////////////////////////////////////////////////////////////////

// To be used like this:
//   mkstr("Some\0string\0with\0embedded\0nuls")
// since std::string("...") doesn't work as expected in that case.
template<size_t N> std::string mkstr(const char (&s)[N])
{
	return std::string(s, N - 1);
}

void TestSerialization::buildTestStrings()
{
	std::ostringstream tmp_os;
	std::wostringstream tmp_os_w;
	std::ostringstream tmp_os_w_encoded;
	for (int i = 0; i < 256; i++) {
		tmp_os << (char)i;
		tmp_os_w << (wchar_t)i;
		tmp_os_w_encoded << (char)0 << (char)i;
	}
	teststring2 = tmp_os.str();
	teststring2_w = tmp_os_w.str();
	teststring2_w_encoded = tmp_os_w_encoded.str();
}

void TestSerialization::testSerializeString()
{
	// Test blank string
	UASSERT(serializeString("") == mkstr("\0\0"));

	// Test basic string
	UASSERT(serializeString("Hello world!") == mkstr("\0\14Hello world!"));

	// Test character range
	UASSERT(serializeString(teststring2) == mkstr("\1\0") + teststring2);
}

void TestSerialization::testDeSerializeString()
{
	// Test deserialize
	{
		std::istringstream is(serializeString(teststring2), std::ios::binary);
		UASSERT(deSerializeString(is) == teststring2);
		UASSERT(!is.eof());
		is.get();
		UASSERT(is.eof());
	}

	// Test deserialize an incomplete length specifier
	{
		std::istringstream is(mkstr("\x53"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeString(is));
	}

	// Test deserialize a string with incomplete data
	{
		std::istringstream is(mkstr("\x00\x55 abcdefg"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeString(is));
	}
}

void TestSerialization::testSerializeWideString()
{
	// Test blank string
	UASSERT(serializeWideString(L"") == mkstr("\0\0"));

	// Test basic string
	UASSERT(serializeWideString(utf8_to_wide("Hello world!")) ==
		mkstr("\0\14\0H\0e\0l\0l\0o\0 \0w\0o\0r\0l\0d\0!"));

	// Test character range
	UASSERT(serializeWideString(teststring2_w) ==
		mkstr("\1\0") + teststring2_w_encoded);
}

void TestSerialization::testDeSerializeWideString()
{
	// Test deserialize
	{
		std::istringstream is(serializeWideString(teststring2_w), std::ios::binary);
		UASSERT(deSerializeWideString(is) == teststring2_w);
		UASSERT(!is.eof());
		is.get();
		UASSERT(is.eof());
	}

	// Test deserialize an incomplete length specifier
	{
		std::istringstream is(mkstr("\x53"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeWideString(is));
	}

	// Test deserialize a string with an incomplete character
	{
		std::istringstream is(mkstr("\x00\x07\0a\0b\0c\0d\0e\0f\0"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeWideString(is));
	}

	// Test deserialize a string with incomplete data
	{
		std::istringstream is(mkstr("\x00\x08\0a\0b\0c\0d\0e\0f"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeWideString(is));
	}
}

void TestSerialization::testSerializeLongString()
{
	// Test blank string
	UASSERT(serializeLongString("") == mkstr("\0\0\0\0"));

	// Test basic string
	UASSERT(serializeLongString("Hello world!") == mkstr("\0\0\0\14Hello world!"));

	// Test character range
	UASSERT(serializeLongString(teststring2) == mkstr("\0\0\1\0") + teststring2);
}

void TestSerialization::testDeSerializeLongString()
{
	// Test deserialize
	{
		std::istringstream is(serializeLongString(teststring2), std::ios::binary);
		UASSERT(deSerializeLongString(is) == teststring2);
		UASSERT(!is.eof());
		is.get();
		UASSERT(is.eof());
	}

	// Test deserialize an incomplete length specifier
	{
		std::istringstream is(mkstr("\x53"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeLongString(is));
	}

	// Test deserialize a string with incomplete data
	{
		std::istringstream is(mkstr("\x00\x00\x00\x05 abc"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeLongString(is));
	}

	// Test deserialize a string with a length too large
	{
		std::istringstream is(mkstr("\xFF\xFF\xFF\xFF blah"), std::ios::binary);
		EXCEPTION_CHECK(SerializationError, deSerializeLongString(is));
	}
}


void TestSerialization::testSerializeJsonString()
{
	// Test blank string
	UASSERT(serializeJsonString("") == "\"\"");

	// Test basic string
	UASSERT(serializeJsonString("Hello world!") == "\"Hello world!\"");

	// MSVC fails when directly using "\\\\"
	std::string backslash = "\\";
	UASSERT(serializeJsonString(teststring2) ==
		mkstr("\"") +
		"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007" +
		"\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f" +
		"\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017" +
		"\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f" +
		" !\\\"" + teststring2.substr(0x23, 0x2f-0x23) +
		"\\/" + teststring2.substr(0x30, 0x5c-0x30) +
		backslash + backslash + teststring2.substr(0x5d, 0x7f-0x5d) + "\\u007f" +
		"\\u0080\\u0081\\u0082\\u0083\\u0084\\u0085\\u0086\\u0087" +
		"\\u0088\\u0089\\u008a\\u008b\\u008c\\u008d\\u008e\\u008f" +
		"\\u0090\\u0091\\u0092\\u0093\\u0094\\u0095\\u0096\\u0097" +
		"\\u0098\\u0099\\u009a\\u009b\\u009c\\u009d\\u009e\\u009f" +
		"\\u00a0\\u00a1\\u00a2\\u00a3\\u00a4\\u00a5\\u00a6\\u00a7" +
		"\\u00a8\\u00a9\\u00aa\\u00ab\\u00ac\\u00ad\\u00ae\\u00af" +
		"\\u00b0\\u00b1\\u00b2\\u00b3\\u00b4\\u00b5\\u00b6\\u00b7" +
		"\\u00b8\\u00b9\\u00ba\\u00bb\\u00bc\\u00bd\\u00be\\u00bf" +
		"\\u00c0\\u00c1\\u00c2\\u00c3\\u00c4\\u00c5\\u00c6\\u00c7" +
		"\\u00c8\\u00c9\\u00ca\\u00cb\\u00cc\\u00cd\\u00ce\\u00cf" +
		"\\u00d0\\u00d1\\u00d2\\u00d3\\u00d4\\u00d5\\u00d6\\u00d7" +
		"\\u00d8\\u00d9\\u00da\\u00db\\u00dc\\u00dd\\u00de\\u00df" +
		"\\u00e0\\u00e1\\u00e2\\u00e3\\u00e4\\u00e5\\u00e6\\u00e7" +
		"\\u00e8\\u00e9\\u00ea\\u00eb\\u00ec\\u00ed\\u00ee\\u00ef" +
		"\\u00f0\\u00f1\\u00f2\\u00f3\\u00f4\\u00f5\\u00f6\\u00f7" +
		"\\u00f8\\u00f9\\u00fa\\u00fb\\u00fc\\u00fd\\u00fe\\u00ff" +
		"\"");

	// Test deserialize
	std::istringstream is(serializeJsonString(teststring2), std::ios::binary);
	UASSERT(deSerializeJsonString(is) == teststring2);
	UASSERT(!is.eof());
	is.get();
	UASSERT(is.eof());
}

void TestSerialization::testSerializeHex()
{
	// Test blank string
	UASSERT(serializeHexString("") == "");
	UASSERT(serializeHexString("", true) == "");

	// Test basic string
	UASSERT(serializeHexString("Hello world!") ==
		"48656c6c6f20776f726c6421");
	UASSERT(serializeHexString("Hello world!", true) ==
		"48 65 6c 6c 6f 20 77 6f 72 6c 64 21");

	// Test binary string
	UASSERT(serializeHexString(mkstr("\x00\x0a\xb0\x63\x1f\x00\xff")) ==
		"000ab0631f00ff");
	UASSERT(serializeHexString(mkstr("\x00\x0a\xb0\x63\x1f\x00\xff"), true) ==
		"00 0a b0 63 1f 00 ff");
}


void TestSerialization::testStreamRead()
{
	std::string datastr(
		(const char *)test_serialized_data,
		sizeof(test_serialized_data));
	std::istringstream is(datastr, std::ios_base::binary);

	UASSERT(readU8(is) == 0x11);
	UASSERT(readU16(is) == 0x2233);
	UASSERT(readU32(is) == 0x44556677);
	UASSERT(readU64(is) == 0x8899AABBCCDDEEFF);

	UASSERT(readS8(is) == -128);
	UASSERT(readS16(is) == 30000);
	UASSERT(readS32(is) == -6);
	UASSERT(readS64(is) == -43);

	UASSERT(readF1000(is) == 53.534f);
	UASSERT(readF1000(is) == -300000.32f);
	UASSERT(readF1000(is) == F1000_MIN);
	UASSERT(readF1000(is) == F1000_MAX);

	UASSERT(deSerializeString(is) == "foobar!");

	UASSERT(readV2S16(is) == v2s16(500, 500));
	UASSERT(readV3S16(is) == v3s16(4207, 604, -30));
	UASSERT(readV2S32(is) == v2s32(1920, 1080));
	UASSERT(readV3S32(is) == v3s32(-400, 6400054, 290549855));
	UASSERT(readV2F1000(is) == v2f(500.656f, 350.345f));

	UASSERT(deSerializeWideString(is) == L"\x02~woof~\x5455");

	UASSERT(readV3F1000(is) == v3f(500, 10024.2f, -192.54f));
	UASSERT(readARGB8(is) == video::SColor(255, 128, 50, 128));

	UASSERT(deSerializeLongString(is) == "some longer string here");

	UASSERT(is.rdbuf()->in_avail() == 2);
	UASSERT(readU16(is) == 0xF00D);
	UASSERT(is.rdbuf()->in_avail() == 0);
}


void TestSerialization::testStreamWrite()
{
	std::ostringstream os(std::ios_base::binary);
	std::string data;

	writeU8(os, 0x11);
	writeU16(os, 0x2233);
	writeU32(os, 0x44556677);
	writeU64(os, 0x8899AABBCCDDEEFF);

	writeS8(os, -128);
	writeS16(os, 30000);
	writeS32(os, -6);
	writeS64(os, -43);

	writeF1000(os, 53.53467f);
	writeF1000(os, -300000.32f);
	writeF1000(os, F1000_MIN);
	writeF1000(os, F1000_MAX);

	os << serializeString("foobar!");

	data = os.str();
	UASSERT(data.size() < sizeof(test_serialized_data));
	UASSERT(!memcmp(&data[0], test_serialized_data, data.size()));

	writeV2S16(os, v2s16(500, 500));
	writeV3S16(os, v3s16(4207, 604, -30));
	writeV2S32(os, v2s32(1920, 1080));
	writeV3S32(os, v3s32(-400, 6400054, 290549855));
	writeV2F1000(os, v2f(500.65661f, 350.34567f));

	os << serializeWideString(L"\x02~woof~\x5455");

	writeV3F1000(os, v3f(500, 10024.2f, -192.54f));
	writeARGB8(os, video::SColor(255, 128, 50, 128));

	os << serializeLongString("some longer string here");

	writeU16(os, 0xF00D);

	data = os.str();
	UASSERT(data.size() == sizeof(test_serialized_data));
	UASSERT(!memcmp(&data[0], test_serialized_data, sizeof(test_serialized_data)));
}


const u8 TestSerialization::test_serialized_data[12 * 13] = {
	0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc,
	0xdd, 0xee, 0xff, 0x80, 0x75, 0x30, 0xff, 0xff, 0xff, 0xfa, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xd5, 0x00, 0x00, 0xd1, 0x1e, 0xee, 0x1e,
	0x5b, 0xc0, 0x80, 0x00, 0x02, 0x80, 0x7F, 0xFF, 0xFD, 0x80, 0x00, 0x07,
	0x66, 0x6f, 0x6f, 0x62, 0x61, 0x72, 0x21, 0x01, 0xf4, 0x01, 0xf4, 0x10,
	0x6f, 0x02, 0x5c, 0xff, 0xe2, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x04,
	0x38, 0xff, 0xff, 0xfe, 0x70, 0x00, 0x61, 0xa8, 0x36, 0x11, 0x51, 0x70,
	0x5f, 0x00, 0x07, 0xa3, 0xb0, 0x00, 0x05, 0x58, 0x89, 0x00, 0x08, 0x00,
	0x02, 0x00, 0x7e, 0x00, 0x77, 0x00, 0x6f, 0x00, 0x6f, 0x00, 0x66, 0x00,
	0x7e, 0x54, 0x55, 0x00, 0x07, 0xa1, 0x20, 0x00, 0x98, 0xf5, 0x08, 0xff,
	0xfd, 0x0f, 0xe4, 0xff, 0x80, 0x32, 0x80, 0x00, 0x00, 0x00, 0x17, 0x73,
	0x6f, 0x6d, 0x65, 0x20, 0x6c, 0x6f, 0x6e, 0x67, 0x65, 0x72, 0x20, 0x73,
	0x74, 0x72, 0x69, 0x6e, 0x67, 0x20, 0x68, 0x65, 0x72, 0x65, 0xF0, 0x0D,
};

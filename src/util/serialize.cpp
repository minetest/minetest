/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "serialize.h"
#include "pointer.h"
#include "porting.h"
#include "util/string.h"
#include "exceptions.h"
#include "irrlichttypes.h"

#include <sstream>
#include <iomanip>
#include <vector>

FloatType g_serialize_f32_type = FLOATTYPE_UNKNOWN;


////
//// String
////

std::string serializeString16(const std::string &plain)
{
	std::string s;
	char buf[2];

	if (plain.size() > STRING_MAX_LEN)
		throw SerializationError("String too long for serializeString16");
	s.reserve(2 + plain.size());

	writeU16((u8 *)&buf[0], plain.size());
	s.append(buf, 2);

	s.append(plain);
	return s;
}

std::string deSerializeString16(std::istream &is)
{
	std::string s;
	char buf[2];

	is.read(buf, 2);
	if (is.gcount() != 2)
		throw SerializationError("deSerializeString16: size not read");

	u16 s_size = readU16((u8 *)buf);
	if (s_size == 0)
		return s;

	s.resize(s_size);
	is.read(&s[0], s_size);
	if (is.gcount() != s_size)
		throw SerializationError("deSerializeString16: couldn't read all chars");

	return s;
}


////
//// Long String
////

std::string serializeString32(const std::string &plain)
{
	std::string s;
	char buf[4];

	if (plain.size() > LONG_STRING_MAX_LEN)
		throw SerializationError("String too long for serializeLongString");
	s.reserve(4 + plain.size());

	writeU32((u8*)&buf[0], plain.size());
	s.append(buf, 4);
	s.append(plain);
	return s;
}

std::string deSerializeString32(std::istream &is)
{
	std::string s;
	char buf[4];

	is.read(buf, 4);
	if (is.gcount() != 4)
		throw SerializationError("deSerializeLongString: size not read");

	u32 s_size = readU32((u8 *)buf);
	if (s_size == 0)
		return s;

	// We don't really want a remote attacker to force us to allocate 4GB...
	if (s_size > LONG_STRING_MAX_LEN) {
		throw SerializationError("deSerializeLongString: "
			"string too long: " + itos(s_size) + " bytes");
	}

	s.resize(s_size);
	is.read(&s[0], s_size);
	if ((u32)is.gcount() != s_size)
		throw SerializationError("deSerializeLongString: couldn't read all chars");

	return s;
}

////
//// JSON
////

std::string serializeJsonString(const std::string &plain)
{
	std::ostringstream os(std::ios::binary);
	os << "\"";

	for (char c : plain) {
		switch (c) {
			case '"':
				os << "\\\"";
				break;
			case '\\':
				os << "\\\\";
				break;
			case '/':
				os << "\\/";
				break;
			case '\b':
				os << "\\b";
				break;
			case '\f':
				os << "\\f";
				break;
			case '\n':
				os << "\\n";
				break;
			case '\r':
				os << "\\r";
				break;
			case '\t':
				os << "\\t";
				break;
			default: {
				if (c >= 32 && c <= 126) {
					os << c;
				} else {
					u32 cnum = (u8)c;
					os << "\\u" << std::hex << std::setw(4)
						<< std::setfill('0') << cnum;
				}
				break;
			}
		}
	}

	os << "\"";
	return os.str();
}

std::string deSerializeJsonString(std::istream &is)
{
	std::ostringstream os(std::ios::binary);
	char c, c2;

	// Parse initial doublequote
	is >> c;
	if (c != '"')
		throw SerializationError("JSON string must start with doublequote");

	// Parse characters
	for (;;) {
		c = is.get();
		if (is.eof())
			throw SerializationError("JSON string ended prematurely");

		if (c == '"') {
			return os.str();
		}

		if (c == '\\') {
			c2 = is.get();
			if (is.eof())
				throw SerializationError("JSON string ended prematurely");
			switch (c2) {
				case 'b':
					os << '\b';
					break;
				case 'f':
					os << '\f';
					break;
				case 'n':
					os << '\n';
					break;
				case 'r':
					os << '\r';
					break;
				case 't':
					os << '\t';
					break;
				case 'u': {
					int hexnumber;
					char hexdigits[4 + 1];

					is.read(hexdigits, 4);
					if (is.eof())
						throw SerializationError("JSON string ended prematurely");
					hexdigits[4] = 0;

					std::istringstream tmp_is(hexdigits, std::ios::binary);
					tmp_is >> std::hex >> hexnumber;
					os << (char)hexnumber;
					break;
				}
				default:
					os << c2;
					break;
			}
		} else {
			os << c;
		}
	}

	return os.str();
}

std::string serializeJsonStringIfNeeded(const std::string &s)
{
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] <= 0x1f || s[i] >= 0x7f || s[i] == ' ' || s[i] == '\"')
			return serializeJsonString(s);
	}
	return s;
}

std::string deSerializeJsonStringIfNeeded(std::istream &is)
{
	std::ostringstream tmp_os;
	bool expect_initial_quote = true;
	bool is_json = false;
	bool was_backslash = false;
	for (;;) {
		char c = is.get();
		if (is.eof())
			break;

		if (expect_initial_quote && c == '"') {
			tmp_os << c;
			is_json = true;
		} else if(is_json) {
			tmp_os << c;
			if (was_backslash)
				was_backslash = false;
			else if (c == '\\')
				was_backslash = true;
			else if (c == '"')
				break; // Found end of string
		} else {
			if (c == ' ') {
				// Found end of word
				is.unget();
				break;
			}

			tmp_os << c;
		}
		expect_initial_quote = false;
	}
	if (is_json) {
		std::istringstream tmp_is(tmp_os.str(), std::ios::binary);
		return deSerializeJsonString(tmp_is);
	}

	return tmp_os.str();
}


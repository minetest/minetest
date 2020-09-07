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
//// BufReader
////

bool BufReader::getStringNoEx(std::string *val)
{
	u16 num_chars;
	if (!getU16NoEx(&num_chars))
		return false;

	if (pos + num_chars > size) {
		pos -= sizeof(num_chars);
		return false;
	}

	val->assign((const char *)data + pos, num_chars);
	pos += num_chars;

	return true;
}

bool BufReader::getWideStringNoEx(std::wstring *val)
{
	u16 num_chars;
	if (!getU16NoEx(&num_chars))
		return false;

	if (pos + num_chars * 2 > size) {
		pos -= sizeof(num_chars);
		return false;
	}

	for (size_t i = 0; i != num_chars; i++) {
		val->push_back(readU16(data + pos));
		pos += 2;
	}

	return true;
}

bool BufReader::getLongStringNoEx(std::string *val)
{
	u32 num_chars;
	if (!getU32NoEx(&num_chars))
		return false;

	if (pos + num_chars > size) {
		pos -= sizeof(num_chars);
		return false;
	}

	val->assign((const char *)data + pos, num_chars);
	pos += num_chars;

	return true;
}

bool BufReader::getRawDataNoEx(void *val, size_t len)
{
	if (pos + len > size)
		return false;

	memcpy(val, data + pos, len);
	pos += len;

	return true;
}


////
//// String
////

std::string serializeString(const std::string &plain)
{
	std::string s;
	char buf[2];

	if (plain.size() > STRING_MAX_LEN)
		throw SerializationError("String too long for serializeString");

	writeU16((u8 *)&buf[0], plain.size());
	s.append(buf, 2);

	s.append(plain);
	return s;
}

std::string deSerializeString(std::istream &is)
{
	std::string s;
	char buf[2];

	is.read(buf, 2);
	if (is.gcount() != 2)
		throw SerializationError("deSerializeString: size not read");

	u16 s_size = readU16((u8 *)buf);
	if (s_size == 0)
		return s;

	Buffer<char> buf2(s_size);
	is.read(&buf2[0], s_size);
	if (is.gcount() != s_size)
		throw SerializationError("deSerializeString: couldn't read all chars");

	s.reserve(s_size);
	s.append(&buf2[0], s_size);
	return s;
}

////
//// Wide String
////

std::string serializeWideString(const std::wstring &plain)
{
	std::string s;
	char buf[2];

	if (plain.size() > WIDE_STRING_MAX_LEN)
		throw SerializationError("String too long for serializeWideString");

	writeU16((u8 *)buf, plain.size());
	s.append(buf, 2);

	for (wchar_t i : plain) {
		writeU16((u8 *)buf, i);
		s.append(buf, 2);
	}
	return s;
}

std::wstring deSerializeWideString(std::istream &is)
{
	std::wstring s;
	char buf[2];

	is.read(buf, 2);
	if (is.gcount() != 2)
		throw SerializationError("deSerializeWideString: size not read");

	u16 s_size = readU16((u8 *)buf);
	if (s_size == 0)
		return s;

	s.reserve(s_size);
	for (u32 i = 0; i < s_size; i++) {
		is.read(&buf[0], 2);
		if (is.gcount() != 2) {
			throw SerializationError(
				"deSerializeWideString: couldn't read all chars");
		}

		wchar_t c16 = readU16((u8 *)buf);
		s.append(&c16, 1);
	}
	return s;
}

////
//// Long String
////

std::string serializeLongString(const std::string &plain)
{
	char buf[4];

	if (plain.size() > LONG_STRING_MAX_LEN)
		throw SerializationError("String too long for serializeLongString");

	writeU32((u8*)&buf[0], plain.size());
	std::string s;
	s.append(buf, 4);
	s.append(plain);
	return s;
}

std::string deSerializeLongString(std::istream &is)
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

	Buffer<char> buf2(s_size);
	is.read(&buf2[0], s_size);
	if ((u32)is.gcount() != s_size)
		throw SerializationError("deSerializeLongString: couldn't read all chars");

	s.reserve(s_size);
	s.append(&buf2[0], s_size);
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

////
//// String/Struct conversions
////

bool deSerializeStringToStruct(std::string valstr,
	std::string format, void *out, size_t olen)
{
	size_t len = olen;
	std::vector<std::string *> strs_alloced;
	std::string *str;
	char *f, *snext;
	size_t pos;

	char *s = &valstr[0];
	char *buf = new char[len];
	char *bufpos = buf;

	char *fmtpos, *fmt = &format[0];
	while ((f = strtok_r(fmt, ",", &fmtpos)) && s) {
		fmt = nullptr;

		bool is_unsigned = false;
		int width = 0;
		char valtype = *f;

		width = (int)strtol(f + 1, &f, 10);
		if (width && valtype == 's')
			valtype = 'i';

		switch (valtype) {
			case 'u':
				is_unsigned = true;
				/* FALLTHROUGH */
			case 'i':
				if (width == 16) {
					bufpos += PADDING(bufpos, u16);
					if ((bufpos - buf) + sizeof(u16) <= len) {
						if (is_unsigned)
							*(u16 *)bufpos = (u16)strtoul(s, &s, 10);
						else
							*(s16 *)bufpos = (s16)strtol(s, &s, 10);
					}
					bufpos += sizeof(u16);
				} else if (width == 32) {
					bufpos += PADDING(bufpos, u32);
					if ((bufpos - buf) + sizeof(u32) <= len) {
						if (is_unsigned)
							*(u32 *)bufpos = (u32)strtoul(s, &s, 10);
						else
							*(s32 *)bufpos = (s32)strtol(s, &s, 10);
					}
					bufpos += sizeof(u32);
				} else if (width == 64) {
					bufpos += PADDING(bufpos, u64);
					if ((bufpos - buf) + sizeof(u64) <= len) {
						if (is_unsigned)
							*(u64 *)bufpos = (u64)strtoull(s, &s, 10);
						else
							*(s64 *)bufpos = (s64)strtoll(s, &s, 10);
					}
					bufpos += sizeof(u64);
				}
				s = strchr(s, ',');
				break;
			case 'b':
				snext = strchr(s, ',');
				if (snext)
					*snext++ = 0;

				bufpos += PADDING(bufpos, bool);
				if ((bufpos - buf) + sizeof(bool) <= len)
					*(bool *)bufpos = is_yes(std::string(s));
				bufpos += sizeof(bool);

				s = snext;
				break;
			case 'f':
				bufpos += PADDING(bufpos, float);
				if ((bufpos - buf) + sizeof(float) <= len)
					*(float *)bufpos = strtof(s, &s);
				bufpos += sizeof(float);

				s = strchr(s, ',');
				break;
			case 's':
				while (*s == ' ' || *s == '\t')
					s++;
				if (*s++ != '"') //error, expected string
					goto fail;
				snext = s;

				while (snext[0] && !(snext[-1] != '\\' && snext[0] == '"'))
					snext++;
				*snext++ = 0;

				bufpos += PADDING(bufpos, std::string *);

				str = new std::string(s);
				pos = 0;
				while ((pos = str->find("\\\"", pos)) != std::string::npos)
					str->erase(pos, 1);

				if ((bufpos - buf) + sizeof(std::string *) <= len)
					*(std::string **)bufpos = str;
				bufpos += sizeof(std::string *);
				strs_alloced.push_back(str);

				s = *snext ? snext + 1 : nullptr;
				break;
			case 'v':
				while (*s == ' ' || *s == '\t')
					s++;
				if (*s++ != '(') //error, expected vector
					goto fail;

				if (width == 2) {
					bufpos += PADDING(bufpos, v2f);

					if ((bufpos - buf) + sizeof(v2f) <= len) {
					v2f *v = (v2f *)bufpos;
						v->X = strtof(s, &s);
						s++;
						v->Y = strtof(s, &s);
					}

					bufpos += sizeof(v2f);
				} else if (width == 3) {
					bufpos += PADDING(bufpos, v3f);
					if ((bufpos - buf) + sizeof(v3f) <= len) {
						v3f *v = (v3f *)bufpos;
						v->X = strtof(s, &s);
						s++;
						v->Y = strtof(s, &s);
						s++;
						v->Z = strtof(s, &s);
					}

					bufpos += sizeof(v3f);
				}
				s = strchr(s, ',');
				break;
			default: //error, invalid format specifier
				goto fail;
		}

		if (s && *s == ',')
			s++;

		if ((size_t)(bufpos - buf) > len) //error, buffer too small
			goto fail;
	}

	if (f && *f) { //error, mismatched number of fields and values
fail:
		for (size_t i = 0; i != strs_alloced.size(); i++)
			delete strs_alloced[i];
		delete[] buf;
		return false;
	}

	memcpy(out, buf, olen);
	delete[] buf;
	return true;
}

// Casts *buf to a signed or unsigned fixed-width integer of 'w' width
#define SIGN_CAST(w, buf) (is_unsigned ? *((u##w *) buf) : *((s##w *) buf))

bool serializeStructToString(std::string *out,
	std::string format, void *value)
{
	std::ostringstream os;
	std::string str;
	char *f;
	size_t strpos;

	char *bufpos = (char *) value;
	char *fmtpos, *fmt = &format[0];
	while ((f = strtok_r(fmt, ",", &fmtpos))) {
		fmt = nullptr;
		bool is_unsigned = false;
		int width = 0;
		char valtype = *f;

		width = (int)strtol(f + 1, &f, 10);
		if (width && valtype == 's')
			valtype = 'i';

		switch (valtype) {
			case 'u':
				is_unsigned = true;
				/* FALLTHROUGH */
			case 'i':
				if (width == 16) {
					bufpos += PADDING(bufpos, u16);
					os << SIGN_CAST(16, bufpos);
					bufpos += sizeof(u16);
				} else if (width == 32) {
					bufpos += PADDING(bufpos, u32);
					os << SIGN_CAST(32, bufpos);
					bufpos += sizeof(u32);
				} else if (width == 64) {
					bufpos += PADDING(bufpos, u64);
					os << SIGN_CAST(64, bufpos);
					bufpos += sizeof(u64);
				}
				break;
			case 'b':
				bufpos += PADDING(bufpos, bool);
				os << std::boolalpha << *((bool *) bufpos);
				bufpos += sizeof(bool);
				break;
			case 'f':
				bufpos += PADDING(bufpos, float);
				os << *((float *) bufpos);
				bufpos += sizeof(float);
				break;
			case 's':
				bufpos += PADDING(bufpos, std::string *);
				str = **((std::string **) bufpos);

				strpos = 0;
				while ((strpos = str.find('"', strpos)) != std::string::npos) {
					str.insert(strpos, 1, '\\');
					strpos += 2;
				}

				os << str;
				bufpos += sizeof(std::string *);
				break;
			case 'v':
				if (width == 2) {
					bufpos += PADDING(bufpos, v2f);
					v2f *v = (v2f *) bufpos;
					os << '(' << v->X << ", " << v->Y << ')';
					bufpos += sizeof(v2f);
				} else {
					bufpos += PADDING(bufpos, v3f);
					v3f *v = (v3f *) bufpos;
					os << '(' << v->X << ", " << v->Y << ", " << v->Z << ')';
					bufpos += sizeof(v3f);
				}
				break;
			default:
				return false;
		}
		os << ", ";
	}
	*out = os.str();

	// Trim off the trailing comma and space
	if (out->size() >= 2)
		out->resize(out->size() - 2);

	return true;
}

#undef SIGN_CAST

////
//// Other
////

std::string serializeHexString(const std::string &data, bool insert_spaces)
{
	std::string result;
	result.reserve(data.size() * (2 + insert_spaces));

	static const char hex_chars[] = "0123456789abcdef";

	const size_t len = data.size();
	for (size_t i = 0; i != len; i++) {
		u8 byte = data[i];
		result.push_back(hex_chars[(byte >> 4) & 0x0F]);
		result.push_back(hex_chars[(byte >> 0) & 0x0F]);
		if (insert_spaces && i != len - 1)
			result.push_back(' ');
	}

	return result;
}

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

#pragma once

#include "irrlichttypes_bloated.h"
#include "exceptions.h" // for SerializationError
#include "debug.h" // for assert
#include "ieee_float.h"

#include "config.h"
#if HAVE_ENDIAN_H
	#ifdef _WIN32
		#define __BYTE_ORDER 0
		#define __LITTLE_ENDIAN 0
		#define __BIG_ENDIAN 1
	#elif defined(__MACH__) && defined(__APPLE__)
		#include <machine/endian.h>
	#elif defined(__FreeBSD__) || defined(__DragonFly__)
		#include <sys/endian.h>
	#else
		#include <endian.h>
	#endif
#endif
#include <cstring> // for memcpy
#include <iostream>
#include <string>
#include <vector>

#define FIXEDPOINT_FACTOR 1000.0f

// 0x7FFFFFFF / 1000.0f is not serializable.
// The limited float precision at this magnitude may cause the result to round
// to a greater value than can be represented by a 32 bit integer when increased
// by a factor of FIXEDPOINT_FACTOR.  As a result, [F1000_MIN..F1000_MAX] does
// not represent the full range, but rather the largest safe range, of values on
// all supported architectures.  Note: This definition makes assumptions on
// platform float-to-int conversion behavior.
#define F1000_MIN ((float)(s32)((-0x7FFFFFFF - 1) / FIXEDPOINT_FACTOR))
#define F1000_MAX ((float)(s32)((0x7FFFFFFF) / FIXEDPOINT_FACTOR))

#define STRING_MAX_LEN 0xFFFF
#define WIDE_STRING_MAX_LEN 0xFFFF
// 64 MB ought to be enough for anybody - Billy G.
#define LONG_STRING_MAX_LEN (64 * 1024 * 1024)


extern FloatType g_serialize_f32_type;

#if HAVE_ENDIAN_H
// use machine native byte swapping routines
// Note: memcpy below is optimized out by modern compilers

inline u16 readU16(const u8 *data)
{
	u16 val;
	memcpy(&val, data, 2);
	return be16toh(val);
}

inline u32 readU32(const u8 *data)
{
	u32 val;
	memcpy(&val, data, 4);
	return be32toh(val);
}

inline u64 readU64(const u8 *data)
{
	u64 val;
	memcpy(&val, data, 8);
	return be64toh(val);
}

inline void writeU16(u8 *data, u16 i)
{
	u16 val = htobe16(i);
	memcpy(data, &val, 2);
}

inline void writeU32(u8 *data, u32 i)
{
	u32 val = htobe32(i);
	memcpy(data, &val, 4);
}

inline void writeU64(u8 *data, u64 i)
{
	u64 val = htobe64(i);
	memcpy(data, &val, 8);
}

#else
// generic byte-swapping implementation

inline u16 readU16(const u8 *data)
{
	return
		((u16)data[0] << 8) | ((u16)data[1] << 0);
}

inline u32 readU32(const u8 *data)
{
	return
		((u32)data[0] << 24) | ((u32)data[1] << 16) |
		((u32)data[2] <<  8) | ((u32)data[3] <<  0);
}

inline u64 readU64(const u8 *data)
{
	return
		((u64)data[0] << 56) | ((u64)data[1] << 48) |
		((u64)data[2] << 40) | ((u64)data[3] << 32) |
		((u64)data[4] << 24) | ((u64)data[5] << 16) |
		((u64)data[6] <<  8) | ((u64)data[7] << 0);
}

inline void writeU16(u8 *data, u16 i)
{
	data[0] = (i >> 8) & 0xFF;
	data[1] = (i >> 0) & 0xFF;
}

inline void writeU32(u8 *data, u32 i)
{
	data[0] = (i >> 24) & 0xFF;
	data[1] = (i >> 16) & 0xFF;
	data[2] = (i >>  8) & 0xFF;
	data[3] = (i >>  0) & 0xFF;
}

inline void writeU64(u8 *data, u64 i)
{
	data[0] = (i >> 56) & 0xFF;
	data[1] = (i >> 48) & 0xFF;
	data[2] = (i >> 40) & 0xFF;
	data[3] = (i >> 32) & 0xFF;
	data[4] = (i >> 24) & 0xFF;
	data[5] = (i >> 16) & 0xFF;
	data[6] = (i >>  8) & 0xFF;
	data[7] = (i >>  0) & 0xFF;
}

#endif // HAVE_ENDIAN_H

//////////////// read routines ////////////////

inline u8 readU8(const u8 *data)
{
	return ((u8)data[0] << 0);
}

inline s8 readS8(const u8 *data)
{
	return (s8)readU8(data);
}

inline s16 readS16(const u8 *data)
{
	return (s16)readU16(data);
}

inline s32 readS32(const u8 *data)
{
	return (s32)readU32(data);
}

inline s64 readS64(const u8 *data)
{
	return (s64)readU64(data);
}

inline f32 readF1000(const u8 *data)
{
	return (f32)readS32(data) / FIXEDPOINT_FACTOR;
}

inline f32 readF32(const u8 *data)
{
	u32 u = readU32(data);

	switch (g_serialize_f32_type) {
	case FLOATTYPE_SYSTEM: {
			f32 f;
			memcpy(&f, &u, 4);
			return f;
		}
	case FLOATTYPE_SLOW:
		return u32Tof32Slow(u);
	case FLOATTYPE_UNKNOWN: // First initialization
		g_serialize_f32_type = getFloatSerializationType();
		return readF32(data);
	}
	throw SerializationError("readF32: Unreachable code");
}

inline video::SColor readARGB8(const u8 *data)
{
	video::SColor p(readU32(data));
	return p;
}

inline v2s16 readV2S16(const u8 *data)
{
	v2s16 p;
	p.X = readS16(&data[0]);
	p.Y = readS16(&data[2]);
	return p;
}

inline v3s16 readV3S16(const u8 *data)
{
	v3s16 p;
	p.X = readS16(&data[0]);
	p.Y = readS16(&data[2]);
	p.Z = readS16(&data[4]);
	return p;
}

inline v2s32 readV2S32(const u8 *data)
{
	v2s32 p;
	p.X = readS32(&data[0]);
	p.Y = readS32(&data[4]);
	return p;
}

inline v3s32 readV3S32(const u8 *data)
{
	v3s32 p;
	p.X = readS32(&data[0]);
	p.Y = readS32(&data[4]);
	p.Z = readS32(&data[8]);
	return p;
}

inline v3f readV3F1000(const u8 *data)
{
	v3f p;
	p.X = readF1000(&data[0]);
	p.Y = readF1000(&data[4]);
	p.Z = readF1000(&data[8]);
	return p;
}

inline v2f readV2F32(const u8 *data)
{
	v2f p;
	p.X = readF32(&data[0]);
	p.Y = readF32(&data[4]);
	return p;
}

inline v3f readV3F32(const u8 *data)
{
	v3f p;
	p.X = readF32(&data[0]);
	p.Y = readF32(&data[4]);
	p.Z = readF32(&data[8]);
	return p;
}

/////////////// write routines ////////////////

inline void writeU8(u8 *data, u8 i)
{
	data[0] = (i >> 0) & 0xFF;
}

inline void writeS8(u8 *data, s8 i)
{
	writeU8(data, (u8)i);
}

inline void writeS16(u8 *data, s16 i)
{
	writeU16(data, (u16)i); 
}

inline void writeS32(u8 *data, s32 i)
{
	writeU32(data, (u32)i);
}

inline void writeS64(u8 *data, s64 i)
{
	writeU64(data, (u64)i);
}

inline void writeF1000(u8 *data, f32 i)
{
	assert(i >= F1000_MIN && i <= F1000_MAX);
	writeS32(data, i * FIXEDPOINT_FACTOR);
}

inline void writeF32(u8 *data, f32 i)
{
	switch (g_serialize_f32_type) {
	case FLOATTYPE_SYSTEM: {
			u32 u;
			memcpy(&u, &i, 4);
			return writeU32(data, u);
		}
	case FLOATTYPE_SLOW:
		return writeU32(data, f32Tou32Slow(i));
	case FLOATTYPE_UNKNOWN: // First initialization
		g_serialize_f32_type = getFloatSerializationType();
		return writeF32(data, i);
	}
	throw SerializationError("writeF32: Unreachable code");
}

inline void writeARGB8(u8 *data, video::SColor p)
{
	writeU32(data, p.color);
}

inline void writeV2S16(u8 *data, v2s16 p)
{
	writeS16(&data[0], p.X);
	writeS16(&data[2], p.Y);
}

inline void writeV3S16(u8 *data, v3s16 p)
{
	writeS16(&data[0], p.X);
	writeS16(&data[2], p.Y);
	writeS16(&data[4], p.Z);
}

inline void writeV2S32(u8 *data, v2s32 p)
{
	writeS32(&data[0], p.X);
	writeS32(&data[4], p.Y);
}

inline void writeV3S32(u8 *data, v3s32 p)
{
	writeS32(&data[0], p.X);
	writeS32(&data[4], p.Y);
	writeS32(&data[8], p.Z);
}

inline void writeV3F1000(u8 *data, v3f p)
{
	writeF1000(&data[0], p.X);
	writeF1000(&data[4], p.Y);
	writeF1000(&data[8], p.Z);
}

inline void writeV2F32(u8 *data, v2f p)
{
	writeF32(&data[0], p.X);
	writeF32(&data[4], p.Y);
}

inline void writeV3F32(u8 *data, v3f p)
{
	writeF32(&data[0], p.X);
	writeF32(&data[4], p.Y);
	writeF32(&data[8], p.Z);
}

////
//// Iostream wrapper for data read/write
////

#define MAKE_STREAM_READ_FXN(T, N, S)    \
	inline T read ## N(std::istream &is) \
	{                                    \
		char buf[S] = {0};               \
		is.read(buf, sizeof(buf));       \
		return read ## N((u8 *)buf);     \
	}

#define MAKE_STREAM_WRITE_FXN(T, N, S)              \
	inline void write ## N(std::ostream &os, T val) \
	{                                               \
		char buf[S];                                \
		write ## N((u8 *)buf, val);                 \
		os.write(buf, sizeof(buf));                 \
	}

MAKE_STREAM_READ_FXN(u8,    U8,       1);
MAKE_STREAM_READ_FXN(u16,   U16,      2);
MAKE_STREAM_READ_FXN(u32,   U32,      4);
MAKE_STREAM_READ_FXN(u64,   U64,      8);
MAKE_STREAM_READ_FXN(s8,    S8,       1);
MAKE_STREAM_READ_FXN(s16,   S16,      2);
MAKE_STREAM_READ_FXN(s32,   S32,      4);
MAKE_STREAM_READ_FXN(s64,   S64,      8);
MAKE_STREAM_READ_FXN(f32,   F1000,    4);
MAKE_STREAM_READ_FXN(f32,   F32,      4);
MAKE_STREAM_READ_FXN(v2s16, V2S16,    4);
MAKE_STREAM_READ_FXN(v3s16, V3S16,    6);
MAKE_STREAM_READ_FXN(v2s32, V2S32,    8);
MAKE_STREAM_READ_FXN(v3s32, V3S32,   12);
MAKE_STREAM_READ_FXN(v3f,   V3F1000, 12);
MAKE_STREAM_READ_FXN(v2f,   V2F32,    8);
MAKE_STREAM_READ_FXN(v3f,   V3F32,   12);
MAKE_STREAM_READ_FXN(video::SColor, ARGB8, 4);

MAKE_STREAM_WRITE_FXN(u8,    U8,       1);
MAKE_STREAM_WRITE_FXN(u16,   U16,      2);
MAKE_STREAM_WRITE_FXN(u32,   U32,      4);
MAKE_STREAM_WRITE_FXN(u64,   U64,      8);
MAKE_STREAM_WRITE_FXN(s8,    S8,       1);
MAKE_STREAM_WRITE_FXN(s16,   S16,      2);
MAKE_STREAM_WRITE_FXN(s32,   S32,      4);
MAKE_STREAM_WRITE_FXN(s64,   S64,      8);
MAKE_STREAM_WRITE_FXN(f32,   F1000,    4);
MAKE_STREAM_WRITE_FXN(f32,   F32,      4);
MAKE_STREAM_WRITE_FXN(v2s16, V2S16,    4);
MAKE_STREAM_WRITE_FXN(v3s16, V3S16,    6);
MAKE_STREAM_WRITE_FXN(v2s32, V2S32,    8);
MAKE_STREAM_WRITE_FXN(v3s32, V3S32,   12);
MAKE_STREAM_WRITE_FXN(v3f,   V3F1000, 12);
MAKE_STREAM_WRITE_FXN(v2f,   V2F32,    8);
MAKE_STREAM_WRITE_FXN(v3f,   V3F32,   12);
MAKE_STREAM_WRITE_FXN(video::SColor, ARGB8, 4);

////
//// More serialization stuff
////

// Creates a string with the length as the first two bytes
std::string serializeString(const std::string &plain);

// Creates a string with the length as the first two bytes from wide string
std::string serializeWideString(const std::wstring &plain);

// Reads a string with the length as the first two bytes
std::string deSerializeString(std::istream &is);

// Reads a wide string with the length as the first two bytes
std::wstring deSerializeWideString(std::istream &is);

// Creates a string with the length as the first four bytes
std::string serializeLongString(const std::string &plain);

// Reads a string with the length as the first four bytes
std::string deSerializeLongString(std::istream &is);

// Creates a string encoded in JSON format (almost equivalent to a C string literal)
std::string serializeJsonString(const std::string &plain);

// Reads a string encoded in JSON format
std::string deSerializeJsonString(std::istream &is);

// If the string contains spaces, quotes or control characters, encodes as JSON.
// Else returns the string unmodified.
std::string serializeJsonStringIfNeeded(const std::string &s);

// Parses a string serialized by serializeJsonStringIfNeeded.
std::string deSerializeJsonStringIfNeeded(std::istream &is);

// Creates a string consisting of the hexadecimal representation of `data`
std::string serializeHexString(const std::string &data, bool insert_spaces=false);

// Creates a string containing comma delimited values of a struct whose layout is
// described by the parameter format
bool serializeStructToString(std::string *out,
	std::string format, void *value);

// Reads a comma delimited string of values into a struct whose layout is
// decribed by the parameter format
bool deSerializeStringToStruct(std::string valstr,
	std::string format, void *out, size_t olen);

////
//// BufReader
////

#define MAKE_BUFREADER_GETNOEX_FXN(T, N, S) \
	inline bool get ## N ## NoEx(T *val)    \
	{                                       \
		if (pos + S > size)                 \
			return false;                   \
		*val = read ## N(data + pos);       \
		pos += S;                           \
		return true;                        \
	}

#define MAKE_BUFREADER_GET_FXN(T, N) \
	inline T get ## N()              \
	{                                \
		T val;                       \
		if (!get ## N ## NoEx(&val)) \
			throw SerializationError("Attempted read past end of data"); \
		return val;                  \
	}

class BufReader {
public:
	BufReader(const u8 *data_, size_t size_) :
		data(data_),
		size(size_)
	{
	}

	MAKE_BUFREADER_GETNOEX_FXN(u8,    U8,       1);
	MAKE_BUFREADER_GETNOEX_FXN(u16,   U16,      2);
	MAKE_BUFREADER_GETNOEX_FXN(u32,   U32,      4);
	MAKE_BUFREADER_GETNOEX_FXN(u64,   U64,      8);
	MAKE_BUFREADER_GETNOEX_FXN(s8,    S8,       1);
	MAKE_BUFREADER_GETNOEX_FXN(s16,   S16,      2);
	MAKE_BUFREADER_GETNOEX_FXN(s32,   S32,      4);
	MAKE_BUFREADER_GETNOEX_FXN(s64,   S64,      8);
	MAKE_BUFREADER_GETNOEX_FXN(f32,   F1000,    4);
	MAKE_BUFREADER_GETNOEX_FXN(v2s16, V2S16,    4);
	MAKE_BUFREADER_GETNOEX_FXN(v3s16, V3S16,    6);
	MAKE_BUFREADER_GETNOEX_FXN(v2s32, V2S32,    8);
	MAKE_BUFREADER_GETNOEX_FXN(v3s32, V3S32,   12);
	MAKE_BUFREADER_GETNOEX_FXN(v3f,   V3F1000, 12);
	MAKE_BUFREADER_GETNOEX_FXN(video::SColor, ARGB8, 4);

	bool getStringNoEx(std::string *val);
	bool getWideStringNoEx(std::wstring *val);
	bool getLongStringNoEx(std::string *val);
	bool getRawDataNoEx(void *data, size_t len);

	MAKE_BUFREADER_GET_FXN(u8,            U8);
	MAKE_BUFREADER_GET_FXN(u16,           U16);
	MAKE_BUFREADER_GET_FXN(u32,           U32);
	MAKE_BUFREADER_GET_FXN(u64,           U64);
	MAKE_BUFREADER_GET_FXN(s8,            S8);
	MAKE_BUFREADER_GET_FXN(s16,           S16);
	MAKE_BUFREADER_GET_FXN(s32,           S32);
	MAKE_BUFREADER_GET_FXN(s64,           S64);
	MAKE_BUFREADER_GET_FXN(f32,           F1000);
	MAKE_BUFREADER_GET_FXN(v2s16,         V2S16);
	MAKE_BUFREADER_GET_FXN(v3s16,         V3S16);
	MAKE_BUFREADER_GET_FXN(v2s32,         V2S32);
	MAKE_BUFREADER_GET_FXN(v3s32,         V3S32);
	MAKE_BUFREADER_GET_FXN(v3f,           V3F1000);
	MAKE_BUFREADER_GET_FXN(video::SColor, ARGB8);
	MAKE_BUFREADER_GET_FXN(std::string,   String);
	MAKE_BUFREADER_GET_FXN(std::wstring,  WideString);
	MAKE_BUFREADER_GET_FXN(std::string,   LongString);

	inline void getRawData(void *val, size_t len)
	{
		if (!getRawDataNoEx(val, len))
			throw SerializationError("Attempted read past end of data");
	}

	inline size_t remaining()
	{
		assert(pos <= size);
		return size - pos;
	}

	const u8 *data;
	size_t size;
	size_t pos = 0;
};

#undef MAKE_BUFREADER_GET_FXN
#undef MAKE_BUFREADER_GETNOEX_FXN


////
//// Vector-based write routines
////

inline void putU8(std::vector<u8> *dest, u8 val)
{
	dest->push_back((val >> 0) & 0xFF);
}

inline void putU16(std::vector<u8> *dest, u16 val)
{
	dest->push_back((val >> 8) & 0xFF);
	dest->push_back((val >> 0) & 0xFF);
}

inline void putU32(std::vector<u8> *dest, u32 val)
{
	dest->push_back((val >> 24) & 0xFF);
	dest->push_back((val >> 16) & 0xFF);
	dest->push_back((val >>  8) & 0xFF);
	dest->push_back((val >>  0) & 0xFF);
}

inline void putU64(std::vector<u8> *dest, u64 val)
{
	dest->push_back((val >> 56) & 0xFF);
	dest->push_back((val >> 48) & 0xFF);
	dest->push_back((val >> 40) & 0xFF);
	dest->push_back((val >> 32) & 0xFF);
	dest->push_back((val >> 24) & 0xFF);
	dest->push_back((val >> 16) & 0xFF);
	dest->push_back((val >>  8) & 0xFF);
	dest->push_back((val >>  0) & 0xFF);
}

inline void putS8(std::vector<u8> *dest, s8 val)
{
	putU8(dest, val);
}

inline void putS16(std::vector<u8> *dest, s16 val)
{
	putU16(dest, val);
}

inline void putS32(std::vector<u8> *dest, s32 val)
{
	putU32(dest, val);
}

inline void putS64(std::vector<u8> *dest, s64 val)
{
	putU64(dest, val);
}

inline void putF1000(std::vector<u8> *dest, f32 val)
{
	putS32(dest, val * FIXEDPOINT_FACTOR);
}

inline void putV2S16(std::vector<u8> *dest, v2s16 val)
{
	putS16(dest, val.X);
	putS16(dest, val.Y);
}

inline void putV3S16(std::vector<u8> *dest, v3s16 val)
{
	putS16(dest, val.X);
	putS16(dest, val.Y);
	putS16(dest, val.Z);
}

inline void putV2S32(std::vector<u8> *dest, v2s32 val)
{
	putS32(dest, val.X);
	putS32(dest, val.Y);
}

inline void putV3S32(std::vector<u8> *dest, v3s32 val)
{
	putS32(dest, val.X);
	putS32(dest, val.Y);
	putS32(dest, val.Z);
}

inline void putV3F1000(std::vector<u8> *dest, v3f val)
{
	putF1000(dest, val.X);
	putF1000(dest, val.Y);
	putF1000(dest, val.Z);
}

inline void putARGB8(std::vector<u8> *dest, video::SColor val)
{
	putU32(dest, val.color);
}

inline void putString(std::vector<u8> *dest, const std::string &val)
{
	if (val.size() > STRING_MAX_LEN)
		throw SerializationError("String too long");

	putU16(dest, val.size());
	dest->insert(dest->end(), val.begin(), val.end());
}

inline void putWideString(std::vector<u8> *dest, const std::wstring &val)
{
	if (val.size() > WIDE_STRING_MAX_LEN)
		throw SerializationError("String too long");

	putU16(dest, val.size());
	for (size_t i = 0; i != val.size(); i++)
		putU16(dest, val[i]);
}

inline void putLongString(std::vector<u8> *dest, const std::string &val)
{
	if (val.size() > LONG_STRING_MAX_LEN)
		throw SerializationError("String too long");

	putU32(dest, val.size());
	dest->insert(dest->end(), val.begin(), val.end());
}

inline void putRawData(std::vector<u8> *dest, const void *src, size_t len)
{
	dest->insert(dest->end(), (u8 *)src, (u8 *)src + len);
}

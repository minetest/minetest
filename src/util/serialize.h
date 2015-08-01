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

#ifndef UTIL_SERIALIZE_HEADER
#define UTIL_SERIALIZE_HEADER

#include "../irrlichttypes_bloated.h"
#include "config.h"
#if HAVE_ENDIAN_H
#include <endian.h>
#include <string.h> // for memcpy
#endif
#include <iostream>
#include <string>

#define FIXEDPOINT_FACTOR 1000.0f
#define FIXEDPOINT_INVFACTOR (1.0f/FIXEDPOINT_FACTOR)

#define STRING_MAX_LEN 0xFFFF
#define WIDE_STRING_MAX_LEN 0xFFFF
// 64 MB ought to be enough for anybody - Billy G.
#define LONG_STRING_MAX_LEN (64 * 1024 * 1024)


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
	return (f32)readS32(data) * FIXEDPOINT_INVFACTOR;
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

inline v2f readV2F1000(const u8 *data)
{
	v2f p;
	p.X = (float)readF1000(&data[0]);
	p.Y = (float)readF1000(&data[4]);
	return p;
}

inline v3f readV3F1000(const u8 *data)
{
	v3f p;
	p.X = (float)readF1000(&data[0]);
	p.Y = (float)readF1000(&data[4]);
	p.Z = (float)readF1000(&data[8]);
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
	writeS32(data, i * FIXEDPOINT_FACTOR);
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

inline void writeV2F1000(u8 *data, v2f p)
{
	writeF1000(&data[0], p.X);
	writeF1000(&data[4], p.Y);
}

inline void writeV3F1000(u8 *data, v3f p)
{
	writeF1000(&data[0], p.X);
	writeF1000(&data[4], p.Y);
	writeF1000(&data[8], p.Z);
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
MAKE_STREAM_READ_FXN(v2s16, V2S16,    4);
MAKE_STREAM_READ_FXN(v3s16, V3S16,    6);
MAKE_STREAM_READ_FXN(v2s32, V2S32,    8);
MAKE_STREAM_READ_FXN(v3s32, V3S32,   12);
MAKE_STREAM_READ_FXN(v2f,   V2F1000,  8);
MAKE_STREAM_READ_FXN(v3f,   V3F1000, 12);
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
MAKE_STREAM_WRITE_FXN(v2s16, V2S16,    4);
MAKE_STREAM_WRITE_FXN(v3s16, V3S16,    6);
MAKE_STREAM_WRITE_FXN(v2s32, V2S32,    8);
MAKE_STREAM_WRITE_FXN(v3s32, V3S32,   12);
MAKE_STREAM_WRITE_FXN(v2f,   V2F1000,  8);
MAKE_STREAM_WRITE_FXN(v3f,   V3F1000, 12);
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

#endif

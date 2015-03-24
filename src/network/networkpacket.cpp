/*
Minetest
Copyright (C) 2015 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "networkpacket.h"
#include "debug.h"
#include "exceptions.h"
#include "util/serialize.h"

NetworkPacket::NetworkPacket(u8 *data, u32 datasize, u16 peer_id):
m_read_offset(0), m_peer_id(peer_id)
{
	m_read_offset = 0;
	m_datasize = datasize - 2;

	// split command and datas
	m_command = readU16(&data[0]);
	m_data = std::vector<u8>(&data[2], &data[2 + m_datasize]);
}

NetworkPacket::NetworkPacket(u16 command, u32 datasize, u16 peer_id):
m_datasize(datasize), m_read_offset(0), m_command(command), m_peer_id(peer_id)
{
	m_data.resize(m_datasize);
}

NetworkPacket::NetworkPacket(u16 command, u32 datasize):
m_datasize(datasize), m_read_offset(0), m_command(command), m_peer_id(0)
{
	m_data.resize(m_datasize);
}

NetworkPacket::~NetworkPacket()
{
	m_data.clear();
}

char* NetworkPacket::getString(u32 from_offset)
{
	if (from_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	return (char*)&m_data[from_offset];
}

void NetworkPacket::putRawString(const char* src, u32 len)
{
	if (m_read_offset + len * sizeof(char) >= m_datasize) {
		m_datasize += len * sizeof(char);
		m_data.resize(m_datasize);
	}

	memcpy(&m_data[m_read_offset], src, len);
	m_read_offset += len;
}

NetworkPacket& NetworkPacket::operator>>(std::string& dst)
{
	u16 strLen = readU16(&m_data[m_read_offset]);
	m_read_offset += sizeof(u16);

	dst.clear();

	if (strLen == 0) {
		return *this;
	}

	dst.reserve(strLen);
	dst.append((char*)&m_data[m_read_offset], strLen);

	m_read_offset += strLen * sizeof(char);
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(std::string src)
{
	u16 msgsize = src.size();
	if (msgsize > 0xFFFF) {
		msgsize = 0xFFFF;
	}

	*this << msgsize;

	if (m_read_offset + msgsize * sizeof(char) >= m_datasize) {
		m_datasize += msgsize * sizeof(char);
		m_data.resize(m_datasize);
	}

	memcpy(&m_data[m_read_offset], src.c_str(), msgsize);
	m_read_offset += msgsize;

	return *this;
}

void NetworkPacket::putLongString(std::string src)
{
	u32 msgsize = src.size();
	if (msgsize > 0xFFFFFFFF) {
		msgsize = 0xFFFFFFFF;
	}

	*this << msgsize;

	if (m_read_offset + msgsize * sizeof(char) >= m_datasize) {
		m_datasize += msgsize * sizeof(char);
		m_data.resize(m_datasize);
	}

	memcpy(&m_data[m_read_offset], src.c_str(), msgsize);
	m_read_offset += msgsize;
}

NetworkPacket& NetworkPacket::operator>>(std::wstring& dst)
{
	u16 strLen = readU16(&m_data[m_read_offset]);
	m_read_offset += sizeof(u16);

	dst.clear();

	if (strLen == 0) {
		return *this;
	}

	dst.reserve(strLen);
	for(u16 i=0; i<strLen; i++) {
		wchar_t c16 = readU16(&m_data[m_read_offset]);
		dst.append(&c16, 1);
		m_read_offset += sizeof(u16);
	}

	return *this;
}

NetworkPacket& NetworkPacket::operator<<(std::wstring src)
{
	u16 msgsize = src.size();
	if (msgsize > 0xFFFF) {
		msgsize = 0xFFFF;
	}

	*this << msgsize;

	// Write string
	for (u16 i=0; i<msgsize; i++) {
		*this << (u16) src[i];
	}

	return *this;
}

std::string NetworkPacket::readLongString()
{
	u32 strLen = readU32(&m_data[m_read_offset]);
	m_read_offset += sizeof(u32);

	if (strLen == 0) {
		return "";
	}

	std::string dst;

	dst.reserve(strLen);
	dst.append((char*)&m_data[m_read_offset], strLen);

	m_read_offset += strLen*sizeof(char);

	return dst;
}

NetworkPacket& NetworkPacket::operator>>(char& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readU8(&m_data[m_read_offset]);

	incrOffset<char>();
	return *this;
}

char NetworkPacket::getChar(u32 offset)
{
	if (offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	return readU8(&m_data[offset]);
}

NetworkPacket& NetworkPacket::operator<<(char src)
{
	checkDataSize<u8>();

	writeU8(&m_data[m_read_offset], src);

	incrOffset<char>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u8 src)
{
	checkDataSize<u8>();

	writeU8(&m_data[m_read_offset], src);

	incrOffset<u8>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(bool src)
{
	checkDataSize<u8>();

	writeU8(&m_data[m_read_offset], src);

	incrOffset<u8>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u16 src)
{
	checkDataSize<u16>();

	writeU16(&m_data[m_read_offset], src);

	incrOffset<u16>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u32 src)
{
	checkDataSize<u32>();

	writeU32(&m_data[m_read_offset], src);

	incrOffset<u32>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u64 src)
{
	checkDataSize<u64>();

	writeU64(&m_data[m_read_offset], src);

	incrOffset<u64>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(float src)
{
	checkDataSize<float>();

	writeF1000(&m_data[m_read_offset], src);

	incrOffset<float>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(bool& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readU8(&m_data[m_read_offset]);

	incrOffset<u8>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(u8& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readU8(&m_data[m_read_offset]);

	incrOffset<u8>();
	return *this;
}

u8 NetworkPacket::getU8(u32 offset)
{
	if (offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	return readU8(&m_data[offset]);
}

u8* NetworkPacket::getU8Ptr(u32 from_offset)
{
	if (m_datasize == 0) {
		return NULL;
	}

	if (from_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	return (u8*)&m_data[from_offset];
}

NetworkPacket& NetworkPacket::operator>>(u16& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readU16(&m_data[m_read_offset]);

	incrOffset<u16>();
	return *this;
}

u16 NetworkPacket::getU16(u32 from_offset)
{
	if (from_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	return readU16(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(u32& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readU32(&m_data[m_read_offset]);

	incrOffset<u32>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(u64& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readU64(&m_data[m_read_offset]);

	incrOffset<u64>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(float& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readF1000(&m_data[m_read_offset]);

	incrOffset<float>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2f& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readV2F1000(&m_data[m_read_offset]);

	incrOffset<v2f>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3f& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readV3F1000(&m_data[m_read_offset]);

	incrOffset<v3f>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(s16& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readS16(&m_data[m_read_offset]);

	incrOffset<s16>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(s16 src)
{
	*this << (u16) src;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(s32& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readS32(&m_data[m_read_offset]);

	incrOffset<s32>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(s32 src)
{
	*this << (u32) src;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3s16& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readV3S16(&m_data[m_read_offset]);

	incrOffset<v3s16>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2s32& dst)
{
	dst = readV2S32(&m_data[m_read_offset]);

	incrOffset<v2s32>();
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3s32& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readV3S32(&m_data[m_read_offset]);

	incrOffset<v3s32>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v2f src)
{
	*this << (float) src.X;
	*this << (float) src.Y;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v3f src)
{
	*this << (float) src.X;
	*this << (float) src.Y;
	*this << (float) src.Z;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v3s16 src)
{
	*this << (s16) src.X;
	*this << (s16) src.Y;
	*this << (s16) src.Z;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v2s32 src)
{
	*this << (s32) src.X;
	*this << (s32) src.Y;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(v3s32 src)
{
	*this << (s32) src.X;
	*this << (s32) src.Y;
	*this << (s32) src.Z;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(video::SColor& dst)
{
	if (m_read_offset >= m_datasize)
		throw SerializationError("Malformed packet read");

	dst = readARGB8(&m_data[m_read_offset]);

	incrOffset<u32>();
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(video::SColor src)
{
	checkDataSize<u32>();

	writeU32(&m_data[m_read_offset], src.color);

	incrOffset<u32>();
	return *this;
}

Buffer<u8> NetworkPacket::oldForgePacket()
{
	SharedBuffer<u8> sb(m_datasize + 2);
	writeU16(&sb[0], m_command);

	u8* datas = getU8Ptr(0);

	if (datas != NULL)
		memcpy(&sb[2], datas, m_datasize);
	return sb;
}

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

void NetworkPacket::checkReadOffset(u32 from_offset, u32 field_size)
{
	if (from_offset + field_size > m_datasize) {
		std::stringstream ss;
		ss << "Reading outside packet (offset: " <<
				from_offset << ", packet size: " << getSize() << ")";
		throw PacketError(ss.str());
	}
}

void NetworkPacket::putRawPacket(u8 *data, u32 datasize, u16 peer_id)
{
	// If a m_command is already set, we are rewriting on same packet
	// This is not permitted
	assert(m_command == 0);

	m_datasize = datasize - 2;
	m_peer_id = peer_id;

	m_data.resize(m_datasize);

	// split command and datas
	m_command = readU16(&data[0]);
	memcpy(m_data.data(), &data[2], m_datasize);
}

const char* NetworkPacket::getString(u32 from_offset)
{
	checkReadOffset(from_offset, 0);

	return (char*)&m_data[from_offset];
}

void NetworkPacket::putRawString(const char* src, u32 len)
{
	if (m_read_offset + len > m_datasize) {
		m_datasize = m_read_offset + len;
		m_data.resize(m_datasize);
	}

	if (len == 0)
		return;

	memcpy(&m_data[m_read_offset], src, len);
	m_read_offset += len;
}

NetworkPacket& NetworkPacket::operator>>(std::string& dst)
{
	checkReadOffset(m_read_offset, 2);
	u16 strLen = readU16(&m_data[m_read_offset]);
	m_read_offset += 2;

	dst.clear();

	if (strLen == 0) {
		return *this;
	}

	checkReadOffset(m_read_offset, strLen);

	dst.reserve(strLen);
	dst.append((char*)&m_data[m_read_offset], strLen);

	m_read_offset += strLen;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(const std::string &src)
{
	u16 msgsize = src.size();
	if (msgsize > STRING_MAX_LEN) {
		throw PacketError("String too long");
	}

	*this << msgsize;

	putRawString(src.c_str(), (u32)msgsize);

	return *this;
}

void NetworkPacket::putLongString(const std::string &src)
{
	u32 msgsize = src.size();
	if (msgsize > LONG_STRING_MAX_LEN) {
		throw PacketError("String too long");
	}

	*this << msgsize;

	putRawString(src.c_str(), msgsize);
}

NetworkPacket& NetworkPacket::operator>>(std::wstring& dst)
{
	checkReadOffset(m_read_offset, 2);
	u16 strLen = readU16(&m_data[m_read_offset]);
	m_read_offset += 2;

	dst.clear();

	if (strLen == 0) {
		return *this;
	}

	checkReadOffset(m_read_offset, strLen * 2);

	dst.reserve(strLen);
	for(u16 i=0; i<strLen; i++) {
		wchar_t c16 = readU16(&m_data[m_read_offset]);
		dst.append(&c16, 1);
		m_read_offset += sizeof(u16);
	}

	return *this;
}

NetworkPacket& NetworkPacket::operator<<(const std::wstring &src)
{
	u16 msgsize = src.size();
	if (msgsize > WIDE_STRING_MAX_LEN) {
		throw PacketError("String too long");
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
	checkReadOffset(m_read_offset, 4);
	u32 strLen = readU32(&m_data[m_read_offset]);
	m_read_offset += 4;

	if (strLen == 0) {
		return "";
	}

	if (strLen > LONG_STRING_MAX_LEN) {
		throw PacketError("String too long");
	}

	checkReadOffset(m_read_offset, strLen);

	std::string dst;

	dst.reserve(strLen);
	dst.append((char*)&m_data[m_read_offset], strLen);

	m_read_offset += strLen;

	return dst;
}

NetworkPacket& NetworkPacket::operator>>(char& dst)
{
	checkReadOffset(m_read_offset, 1);

	dst = readU8(&m_data[m_read_offset]);

	m_read_offset += 1;
	return *this;
}

char NetworkPacket::getChar(u32 offset)
{
	checkReadOffset(offset, 1);

	return readU8(&m_data[offset]);
}

NetworkPacket& NetworkPacket::operator<<(char src)
{
	checkDataSize(1);

	writeU8(&m_data[m_read_offset], src);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u8 src)
{
	checkDataSize(1);

	writeU8(&m_data[m_read_offset], src);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(bool src)
{
	checkDataSize(1);

	writeU8(&m_data[m_read_offset], src);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u16 src)
{
	checkDataSize(2);

	writeU16(&m_data[m_read_offset], src);

	m_read_offset += 2;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u32 src)
{
	checkDataSize(4);

	writeU32(&m_data[m_read_offset], src);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(u64 src)
{
	checkDataSize(8);

	writeU64(&m_data[m_read_offset], src);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(float src)
{
	checkDataSize(4);

	writeF1000(&m_data[m_read_offset], src);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(bool& dst)
{
	checkReadOffset(m_read_offset, 1);

	dst = readU8(&m_data[m_read_offset]);

	m_read_offset += 1;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(u8& dst)
{
	checkReadOffset(m_read_offset, 1);

	dst = readU8(&m_data[m_read_offset]);

	m_read_offset += 1;
	return *this;
}

u8 NetworkPacket::getU8(u32 offset)
{
	checkReadOffset(offset, 1);

	return readU8(&m_data[offset]);
}

u8* NetworkPacket::getU8Ptr(u32 from_offset)
{
	if (m_datasize == 0) {
		return NULL;
	}

	checkReadOffset(from_offset, 1);

	return (u8*)&m_data[from_offset];
}

NetworkPacket& NetworkPacket::operator>>(u16& dst)
{
	checkReadOffset(m_read_offset, 2);

	dst = readU16(&m_data[m_read_offset]);

	m_read_offset += 2;
	return *this;
}

u16 NetworkPacket::getU16(u32 from_offset)
{
	checkReadOffset(from_offset, 2);

	return readU16(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(u32& dst)
{
	checkReadOffset(m_read_offset, 4);

	dst = readU32(&m_data[m_read_offset]);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(u64& dst)
{
	checkReadOffset(m_read_offset, 8);

	dst = readU64(&m_data[m_read_offset]);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(float& dst)
{
	checkReadOffset(m_read_offset, 4);

	dst = readF1000(&m_data[m_read_offset]);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2f& dst)
{
	checkReadOffset(m_read_offset, 8);

	dst = readV2F1000(&m_data[m_read_offset]);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3f& dst)
{
	checkReadOffset(m_read_offset, 12);

	dst = readV3F1000(&m_data[m_read_offset]);

	m_read_offset += 12;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(s16& dst)
{
	checkReadOffset(m_read_offset, 2);

	dst = readS16(&m_data[m_read_offset]);

	m_read_offset += 2;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(s16 src)
{
	*this << (u16) src;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(s32& dst)
{
	checkReadOffset(m_read_offset, 4);

	dst = readS32(&m_data[m_read_offset]);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(s32 src)
{
	*this << (u32) src;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3s16& dst)
{
	checkReadOffset(m_read_offset, 6);

	dst = readV3S16(&m_data[m_read_offset]);

	m_read_offset += 6;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2s32& dst)
{
	checkReadOffset(m_read_offset, 8);

	dst = readV2S32(&m_data[m_read_offset]);

	m_read_offset += 8;
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3s32& dst)
{
	checkReadOffset(m_read_offset, 12);

	dst = readV3S32(&m_data[m_read_offset]);

	m_read_offset += 12;
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
	checkReadOffset(m_read_offset, 4);

	dst = readARGB8(&m_data[m_read_offset]);

	m_read_offset += 4;
	return *this;
}

NetworkPacket& NetworkPacket::operator<<(video::SColor src)
{
	checkDataSize(4);

	writeU32(&m_data[m_read_offset], src.color);

	m_read_offset += 4;
	return *this;
}

Buffer<u8> NetworkPacket::oldForgePacket()
{
	Buffer<u8> sb(m_datasize + 2);
	writeU16(&sb[0], m_command);

	u8* datas = getU8Ptr(0);

	if (datas != NULL)
		memcpy(&sb[2], datas, m_datasize);
	return sb;
}

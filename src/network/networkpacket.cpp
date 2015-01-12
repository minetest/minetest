/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
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
#include "util/serialize.h"

NetworkPacket::NetworkPacket(u8 *data, u32 datasize, u16 peer_id):
m_peer_id(peer_id)
{
	m_read_offset = 0;
	m_datasize = datasize - 2;

	// Copy data packet to remove opcode
	m_data = new u8[m_datasize];

	memcpy(m_data, &data[2], m_datasize);
}

NetworkPacket::~NetworkPacket()
{
	delete [] m_data;
}

char* NetworkPacket::getString(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return (char*)&m_data[from_offset];
}

char NetworkPacket::getChar(u32 offset)
{
	assert(offset < m_datasize);

	return m_data[offset];
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

	m_read_offset += strLen*sizeof(char);
	return *this;
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
	dst = getChar(m_read_offset);

	m_read_offset += sizeof(char);
	return *this;
}

u8* NetworkPacket::getU8Ptr(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return (u8*)&m_data[from_offset];
}

u8 NetworkPacket::getU8(u32 offset)
{
	assert(offset < m_datasize);

	return m_data[offset];
}

NetworkPacket& NetworkPacket::operator>>(u8& dst)
{
	assert(m_read_offset < m_datasize);
	dst = m_data[m_read_offset];

	m_read_offset += sizeof(u8);
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(bool& dst)
{
	assert(m_read_offset < m_datasize);
	dst = m_data[m_read_offset];

	m_read_offset += sizeof(u8);
	return *this;
}

u16 NetworkPacket::getU16(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readU16(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(u16& dst)
{
	dst = getU16(m_read_offset);

	m_read_offset += sizeof(u16);
	return *this;
}

u32 NetworkPacket::getU32(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readU32(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(u32& dst)
{
	dst = getU32(m_read_offset);

	m_read_offset += sizeof(u32);
	return *this;
}

u64 NetworkPacket::getU64(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readU64(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(u64& dst)
{
	dst = getU64(m_read_offset);

	m_read_offset += sizeof(u64);
	return *this;
}

float NetworkPacket::getF1000(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readF1000(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(float& dst)
{
	dst = getF1000(m_read_offset);

	m_read_offset += sizeof(float);
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2f& dst)
{
	assert(m_read_offset < m_datasize);

	dst = readV2F1000(&m_data[m_read_offset]);

	m_read_offset += sizeof(v2f);
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v3f& dst)
{
	assert(m_read_offset < m_datasize);

	dst = readV3F1000(&m_data[m_read_offset]);

	m_read_offset += sizeof(v3f);
	return *this;
}

s16 NetworkPacket::getS16(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readS16(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(s16& dst)
{
	dst = getS16(m_read_offset);

	m_read_offset += sizeof(s16);
	return *this;
}

s32 NetworkPacket::getS32(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readS32(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(s32& dst)
{
	dst = getS32(m_read_offset);

	m_read_offset += sizeof(s32);
	return *this;
}

NetworkPacket& NetworkPacket::operator>>(v2s32& dst)
{
	dst = readV2S32(&m_data[m_read_offset]);

	m_read_offset += sizeof(v2s32);
	return *this;
}

v3s16 NetworkPacket::getV3S16(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readV3S16(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(v3s16& dst)
{
	dst = getV3S16(m_read_offset);

	m_read_offset += sizeof(v3s16);
	return *this;
}

v3s32 NetworkPacket::getV3S32(u32 from_offset)
{
	assert(from_offset < m_datasize);

	return readV3S32(&m_data[from_offset]);
}

NetworkPacket& NetworkPacket::operator>>(v3s32& dst)
{
	dst = getV3S32(m_read_offset);

	m_read_offset += sizeof(v3s32);
	return *this;
}

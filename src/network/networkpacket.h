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

#ifndef NETWORKPACKET_HEADER
#define NETWORKPACKET_HEADER

#include "util/pointer.h"
#include "util/numeric.h"
#include "networkprotocol.h"

class NetworkPacket
{

public:
		NetworkPacket(u16 command, u32 datasize, u16 peer_id);
		NetworkPacket(u16 command, u32 datasize);
		NetworkPacket(): m_datasize(0), m_read_offset(0), m_command(0),
				m_peer_id(0) {}
		~NetworkPacket();

		void putRawPacket(u8 *data, u32 datasize, u16 peer_id);

		// Getters
		u32 getSize() { return m_datasize; }
		u16 getPeerId() { return m_peer_id; }
		u16 getCommand() { return m_command; }
		const u32 getRemainingBytes() const { return m_datasize - m_read_offset; }
		const char* getRemainingString() { return getString(m_read_offset); }

		// Returns a c-string without copying.
		// A better name for this would be getRawString()
		const char* getString(u32 from_offset);
		// major difference to putCString(): doesn't write len into the buffer
		void putRawString(const char* src, u32 len);
		void putRawString(const std::string &src)
			{ putRawString(src.c_str(), src.size()); }

		NetworkPacket& operator>>(std::string& dst);
		NetworkPacket& operator<<(const std::string &src);

		void putLongString(const std::string &src);

		NetworkPacket& operator>>(std::wstring& dst);
		NetworkPacket& operator<<(const std::wstring &src);

		std::string readLongString();

		char getChar(u32 offset);
		NetworkPacket& operator>>(char& dst);
		NetworkPacket& operator<<(char src);

		NetworkPacket& operator>>(bool& dst);
		NetworkPacket& operator<<(bool src);

		u8 getU8(u32 offset);

		NetworkPacket& operator>>(u8& dst);
		NetworkPacket& operator<<(u8 src);

		u8* getU8Ptr(u32 offset);

		u16 getU16(u32 from_offset);
		NetworkPacket& operator>>(u16& dst);
		NetworkPacket& operator<<(u16 src);

		NetworkPacket& operator>>(u32& dst);
		NetworkPacket& operator<<(u32 src);

		NetworkPacket& operator>>(u64& dst);
		NetworkPacket& operator<<(u64 src);

		NetworkPacket& operator>>(float& dst);
		NetworkPacket& operator<<(float src);

		NetworkPacket& operator>>(v2f& dst);
		NetworkPacket& operator<<(v2f src);

		NetworkPacket& operator>>(v3f& dst);
		NetworkPacket& operator<<(v3f src);

		NetworkPacket& operator>>(s16& dst);
		NetworkPacket& operator<<(s16 src);

		NetworkPacket& operator>>(s32& dst);
		NetworkPacket& operator<<(s32 src);

		NetworkPacket& operator>>(v2s32& dst);
		NetworkPacket& operator<<(v2s32 src);

		NetworkPacket& operator>>(v3s16& dst);
		NetworkPacket& operator<<(v3s16 src);

		NetworkPacket& operator>>(v3s32& dst);
		NetworkPacket& operator<<(v3s32 src);

		NetworkPacket& operator>>(video::SColor& dst);
		NetworkPacket& operator<<(video::SColor src);

		// Temp, we remove SharedBuffer when migration finished
		Buffer<u8> oldForgePacket();
private:
		void checkReadOffset(u32 from_offset, u32 field_size);

		inline void checkDataSize(u32 field_size)
		{
			if (m_read_offset + field_size > m_datasize) {
				m_datasize = m_read_offset + field_size;
				m_data.resize(m_datasize);
			}
		}

		std::vector<u8> m_data;
		u32 m_datasize;
		u32 m_read_offset;
		u16 m_command;
		u16 m_peer_id;
};

#endif

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

#ifndef NETWORKPACKET_HEADER
#define NETWORKPACKET_HEADER

#include "util/numeric.h"
#include "networkprotocol.h"

class NetworkPacket
{

public:
		NetworkPacket(u8 *data, u32 datasize, u16 peer_id);
		~NetworkPacket();

		// Getters
		u32 getSize() { return m_datasize; }
		u16 getPeerId() { return m_peer_id; }

		// Data extractors
		char* getString(u32 from_offset);
		NetworkPacket& operator>>(std::string& dst);
		NetworkPacket& operator>>(std::wstring& dst);
		std::string readLongString();

		char getChar(u32 offset);
		NetworkPacket& operator>>(char& dst);

		NetworkPacket& operator>>(bool& dst);

		u8 getU8(u32 offset);
		NetworkPacket& operator>>(u8& dst);

		u8* getU8Ptr(u32 offset);
		u16 getU16(u32 from_offset);
		NetworkPacket& operator>>(u16& dst);
		u32 getU32(u32 from_offset);
		NetworkPacket& operator>>(u32& dst);
		u64 getU64(u32 from_offset);
		NetworkPacket& operator>>(u64& dst);

		float getF1000(u32 offset);
		NetworkPacket& operator>>(float& dst);
		NetworkPacket& operator>>(v2f& dst);
		NetworkPacket& operator>>(v3f& dst);

		s16 getS16(u32 from_offset);
		NetworkPacket& operator>>(s16& dst);
		s32 getS32(u32 from_offset);
		NetworkPacket& operator>>(s32& dst);

		NetworkPacket& operator>>(v2s32& dst);

		v3s16 getV3S16(u32 from_offset);
		NetworkPacket& operator>>(v3s16& dst);

		v3s32 getV3S32(u32 from_offset);
		NetworkPacket& operator>>(v3s32& dst);

protected:
		u8 *m_data;
		u32 m_datasize;
		u32 m_read_offset;
private:
		u16 m_peer_id;
};

#endif

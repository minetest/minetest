/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CLIENTSERVER_HEADER
#define CLIENTSERVER_HEADER

#include "utility.h"

#define PROTOCOL_ID 0x4f457403

enum ToClientCommand
{
	TOCLIENT_INIT = 0x10,
	/*
		Server's reply to TOSERVER_INIT.
		Sent second after connected.

		[0] u16 TOSERVER_INIT
		[2] u8 deployed version
		[3] v3s16 player's position + v3f(0,BS/2,0) floatToInt'd 
		([4] u64 map seed (new as of 2011-02-27))

		NOTE: The position in here is deprecated; position is
		      explicitly sent afterwards
	*/

	TOCLIENT_BLOCKDATA = 0x20, //TODO: Multiple blocks
	TOCLIENT_ADDNODE = 0x21,
	TOCLIENT_REMOVENODE = 0x22,
	
	TOCLIENT_PLAYERPOS = 0x23, // Obsolete
	/*
		[0] u16 command
		// Followed by an arbitary number of these:
		// Number is determined from packet length.
		[N] u16 peer_id
		[N+2] v3s32 position*100
		[N+2+12] v3s32 speed*100
		[N+2+12+12] s32 pitch*100
		[N+2+12+12+4] s32 yaw*100
	*/

	TOCLIENT_PLAYERINFO = 0x24,
	/*
		[0] u16 command
		// Followed by an arbitary number of these:
		// Number is determined from packet length.
		[N] u16 peer_id
		[N] char[20] name
	*/
	
	TOCLIENT_OPT_BLOCK_NOT_FOUND = 0x25, // Obsolete

	TOCLIENT_SECTORMETA = 0x26, // Obsolete
	/*
		[0] u16 command
		[2] u8 sector count
		[3...] v2s16 pos + sector metadata
	*/

	TOCLIENT_INVENTORY = 0x27,
	/*
		[0] u16 command
		[2] serialized inventory
	*/
	
	TOCLIENT_OBJECTDATA = 0x28,
	/*
		Sent as unreliable.

		u16 command
		u16 number of player positions
		for each player:
			v3s32 position*100
			v3s32 speed*100
			s32 pitch*100
			s32 yaw*100
		u16 count of blocks
		for each block:
			v3s16 blockpos
			block objects
	*/

	TOCLIENT_TIME_OF_DAY = 0x29,
	/*
		u16 command
		u16 time (0-23999)
	*/

	TOCLIENT_CHAT_MESSAGE = 0x30,
	/*
		u16 command
		u16 length
		wstring message
	*/

	TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD = 0x31,
	/*
		u16 command
		u16 count of removed objects
		for all removed objects {
			u16 id
		}
		u16 count of added objects
		for all added objects {
			u16 id
			u8 type
			u16 initialization data length
			string initialization data
		}
	*/
	
	TOCLIENT_ACTIVE_OBJECT_MESSAGES = 0x32,
	/*
		u16 command
		for all objects
		{
			u16 id
			u16 message length
			string message
		}
	*/

	TOCLIENT_HP = 0x33,
	/*
		u16 command
		u8 hp
	*/

	TOCLIENT_MOVE_PLAYER = 0x34,
	/*
		u16 command
		v3f1000 player position
		f1000 player pitch
		f1000 player yaw
	*/

	TOCLIENT_ACCESS_DENIED = 0x35,
	/*
		u16 command
	*/
};

enum ToServerCommand
{
	TOSERVER_INIT=0x10,
	/*
		Sent first after connected.

		[0] u16 TOSERVER_INIT
		[2] u8 SER_FMT_VER_HIGHEST
		[3] u8[20] player_name
		[23] u8[28] password
	*/

	TOSERVER_INIT2 = 0x11,
	/*
		Sent as an ACK for TOCLIENT_INIT.
		After this, the server can send data.

		[0] u16 TOSERVER_INIT2
	*/

	TOSERVER_GETBLOCK=0x20, // Obsolete
	TOSERVER_ADDNODE = 0x21, // Obsolete
	TOSERVER_REMOVENODE = 0x22, // Obsolete

	TOSERVER_PLAYERPOS = 0x23,
	/*
		[0] u16 command
		[2] v3s32 position*100
		[2+12] v3s32 speed*100
		[2+12+12] s32 pitch*100
		[2+12+12+4] s32 yaw*100
	*/

	TOSERVER_GOTBLOCKS = 0x24,
	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	TOSERVER_DELETEDBLOCKS = 0x25,
	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	TOSERVER_ADDNODE_FROM_INVENTORY = 0x26, // Obsolete
	/*
		[0] u16 command
		[2] v3s16 pos
		[8] u16 i
	*/

	TOSERVER_CLICK_OBJECT = 0x27,
	/*
		length: 13
		[0] u16 command
		[2] u8 button (0=left, 1=right)
		[3] v3s16 blockpos
		[9] s16 id
		[11] u16 item
	*/

	TOSERVER_GROUND_ACTION = 0x28,
	/*
		length: 17
		[0] u16 command
		[2] u8 action
		[3] v3s16 nodepos_undersurface
		[9] v3s16 nodepos_abovesurface
		[15] u16 item
		actions:
		0: start digging (from undersurface)
		1: place block (to abovesurface)
		2: stop digging (all parameters ignored)
		3: digging completed
	*/
	
	TOSERVER_RELEASE = 0x29, // Obsolete

	TOSERVER_SIGNTEXT = 0x30, // Old signs
	/*
		u16 command
		v3s16 blockpos
		s16 id
		u16 textlen
		textdata
	*/

	TOSERVER_INVENTORY_ACTION = 0x31,
	/*
		See InventoryAction in inventory.h
	*/

	TOSERVER_CHAT_MESSAGE = 0x32,
	/*
		u16 command
		u16 length
		wstring message
	*/

	TOSERVER_SIGNNODETEXT = 0x33,
	/*
		u16 command
		v3s16 p
		u16 textlen
		textdata
	*/

	TOSERVER_CLICK_ACTIVEOBJECT = 0x34,
	/*
		length: 7
		[0] u16 command
		[2] u8 button (0=left, 1=right)
		[3] u16 id
		[5] u16 item
	*/
	
	TOSERVER_DAMAGE = 0x35,
	/*
		u16 command
		u8 amount
	*/
};

inline SharedBuffer<u8> makePacket_TOCLIENT_TIME_OF_DAY(u16 time)
{
	SharedBuffer<u8> data(2+2);
	writeU16(&data[0], TOCLIENT_TIME_OF_DAY);
	writeU16(&data[2], time);
	return data;
}

#endif


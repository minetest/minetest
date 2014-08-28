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

#ifndef CLIENTSERVER_HEADER
#define CLIENTSERVER_HEADER
#include "util/string.h"

/*
	changes by PROTOCOL_VERSION:

	PROTOCOL_VERSION 3:
		Base for writing changes here
	PROTOCOL_VERSION 4:
		Add TOCLIENT_MEDIA
		Add TOCLIENT_TOOLDEF
		Add TOCLIENT_NODEDEF
		Add TOCLIENT_CRAFTITEMDEF
		Add TOSERVER_INTERACT
		Obsolete TOSERVER_CLICK_ACTIVEOBJECT
		Obsolete TOSERVER_GROUND_ACTION
	PROTOCOL_VERSION 5:
		Make players to be handled mostly as ActiveObjects
	PROTOCOL_VERSION 6:
		Only non-cached textures are sent
	PROTOCOL_VERSION 7:
		Add TOCLIENT_ITEMDEF
		Obsolete TOCLIENT_TOOLDEF
		Obsolete TOCLIENT_CRAFTITEMDEF
		Compress the contents of TOCLIENT_ITEMDEF and TOCLIENT_NODEDEF
	PROTOCOL_VERSION 8:
		Digging based on item groups
		Many things
	PROTOCOL_VERSION 9:
		ContentFeatures and NodeDefManager use a different serialization
		    format; better for future version cross-compatibility
		Many things
	PROTOCOL_VERSION 10:
		TOCLIENT_PRIVILEGES
		Version raised to force 'fly' and 'fast' privileges into effect.
		Node metadata change (came in later; somewhat incompatible)
	PROTOCOL_VERSION 11:
		TileDef in ContentFeatures
		Nodebox drawtype
		(some dev snapshot)
		TOCLIENT_INVENTORY_FORMSPEC
		(0.4.0, 0.4.1)
	PROTOCOL_VERSION 12:
		TOSERVER_INVENTORY_FIELDS
		16-bit node ids
		TOCLIENT_DETACHED_INVENTORY
	PROTOCOL_VERSION 13:
		InventoryList field "Width" (deserialization fails with old versions)
	PROTOCOL_VERSION 14:
		Added transfer of player pressed keys to the server
		Added new messages for mesh and bone animation, as well as attachments
		GENERIC_CMD_SET_ANIMATION
		GENERIC_CMD_SET_BONE_POSITION
		GENERIC_CMD_SET_ATTACHMENT
	PROTOCOL_VERSION 15:
		Serialization format changes
	PROTOCOL_VERSION 16:
		TOCLIENT_SHOW_FORMSPEC
	PROTOCOL_VERSION 17:
		Serialization format change: include backface_culling flag in TileDef
		Added rightclickable field in nodedef
		TOCLIENT_SPAWN_PARTICLE
		TOCLIENT_ADD_PARTICLESPAWNER
		TOCLIENT_DELETE_PARTICLESPAWNER
	PROTOCOL_VERSION 18:
		damageGroups added to ToolCapabilities
		sound_place added to ItemDefinition
	PROTOCOL_VERSION 19:
		GENERIC_CMD_SET_PHYSICS_OVERRIDE
	PROTOCOL_VERSION 20:
		TOCLIENT_HUDADD
		TOCLIENT_HUDRM
		TOCLIENT_HUDCHANGE
		TOCLIENT_HUD_SET_FLAGS
	PROTOCOL_VERSION 21:
		TOCLIENT_BREATH
		TOSERVER_BREATH
		range added to ItemDefinition
		drowning, leveled and liquid_range added to ContentFeatures
		stepheight and collideWithObjects added to object properties
		version, heat and humidity transfer in MapBock
		automatic_face_movement_dir and automatic_face_movement_dir_offset
			added to object properties
	PROTOCOL_VERSION 22:
		add swap_node
	PROTOCOL_VERSION 23:
		TOSERVER_CLIENT_READY
	PROTOCOL_VERSION 24:
		ContentFeatures version 7
		ContentFeatures: change number of special tiles to 6 (CF_SPECIAL_COUNT)
*/

#define LATEST_PROTOCOL_VERSION 24

// Server's supported network protocol range
#define SERVER_PROTOCOL_VERSION_MIN 13
#define SERVER_PROTOCOL_VERSION_MAX LATEST_PROTOCOL_VERSION

// Client's supported network protocol range
#define CLIENT_PROTOCOL_VERSION_MIN 13
#define CLIENT_PROTOCOL_VERSION_MAX LATEST_PROTOCOL_VERSION

// Constant that differentiates the protocol from random data and other protocols
#define PROTOCOL_ID 0x4f457403

#define PASSWORD_SIZE 28       // Maximum password length. Allows for
                               // base64-encoded SHA-1 (27+\0).

#define FORMSPEC_API_VERSION 1
#define FORMSPEC_VERSION_STRING "formspec_version[" TOSTRING(FORMSPEC_API_VERSION) "]"

#define TEXTURENAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-"

enum ToClientCommand
{
	TOCLIENT_INIT = 0x10,
	/*
		Server's reply to TOSERVER_INIT.
		Sent second after connected.

		[0] u16 TOSERVER_INIT
		[2] u8 deployed version
		[3] v3s16 player's position + v3f(0,BS/2,0) floatToInt'd
		[12] u64 map seed (new as of 2011-02-27)
		[20] f1000 recommended send interval (in seconds) (new as of 14)

		NOTE: The position in here is deprecated; position is
		      explicitly sent afterwards
	*/

	TOCLIENT_BLOCKDATA = 0x20, //TODO: Multiple blocks
	TOCLIENT_ADDNODE = 0x21,
	/*
		u16 command
		v3s16 position
		serialized mapnode
		u8 keep_metadata // Added in protocol version 22
	*/
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

	TOCLIENT_PLAYERINFO = 0x24, // Obsolete
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

	TOCLIENT_OBJECTDATA = 0x28, // Obsolete
	/*
		Sent as unreliable.

		u16 command
		u16 number of player positions
		for each player:
			u16 peer_id
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
		Added in a later version:
		f1000 time_speed
	*/

	// (oops, there is some gap here)

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
			u32 initialization data length
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
		u16 reason_length
		wstring reason
	*/

	TOCLIENT_PLAYERITEM = 0x36, // Obsolete
	/*
		u16 command
		u16 count of player items
		for all player items {
			u16 peer id
			u16 length of serialized item
			string serialized item
		}
	*/

	TOCLIENT_DEATHSCREEN = 0x37,
	/*
		u16 command
		u8 bool set camera point target
		v3f1000 camera point target (to point the death cause or whatever)
	*/

	TOCLIENT_MEDIA = 0x38,
	/*
		u16 command
		u16 total number of texture bunches
		u16 index of this bunch
		u32 number of files in this bunch
		for each file {
			u16 length of name
			string name
			u32 length of data
			data
		}
		u16 length of remote media server url (if applicable)
		string url
	*/

	TOCLIENT_TOOLDEF = 0x39,
	/*
		u16 command
		u32 length of the next item
		serialized ToolDefManager
	*/

	TOCLIENT_NODEDEF = 0x3a,
	/*
		u16 command
		u32 length of the next item
		serialized NodeDefManager
	*/

	TOCLIENT_CRAFTITEMDEF = 0x3b,
	/*
		u16 command
		u32 length of the next item
		serialized CraftiItemDefManager
	*/

	TOCLIENT_ANNOUNCE_MEDIA = 0x3c,

	/*
		u16 command
		u32 number of files
		for each texture {
			u16 length of name
			string name
			u16 length of sha1_digest
			string sha1_digest
		}
	*/

	TOCLIENT_ITEMDEF = 0x3d,
	/*
		u16 command
		u32 length of next item
		serialized ItemDefManager
	*/

	TOCLIENT_PLAY_SOUND = 0x3f,
	/*
		u16 command
		s32 sound_id
		u16 len
		u8[len] sound name
		s32 gain*1000
		u8 type (0=local, 1=positional, 2=object)
		s32[3] pos_nodes*10000
		u16 object_id
		u8 loop (bool)
	*/

	TOCLIENT_STOP_SOUND = 0x40,
	/*
		u16 command
		s32 sound_id
	*/

	TOCLIENT_PRIVILEGES = 0x41,
	/*
		u16 command
		u16 number of privileges
		for each privilege
			u16 len
			u8[len] privilege
	*/

	TOCLIENT_INVENTORY_FORMSPEC = 0x42,
	/*
		u16 command
		u32 len
		u8[len] formspec
	*/

	TOCLIENT_DETACHED_INVENTORY = 0x43,
	/*
		[0] u16 command
		u16 len
		u8[len] name
		[2] serialized inventory
	*/

	TOCLIENT_SHOW_FORMSPEC = 0x44,
	/*
		[0] u16 command
		u32 len
		u8[len] formspec
		u16 len
		u8[len] formname
	*/

	TOCLIENT_MOVEMENT = 0x45,
	/*
		u16 command
		f1000 movement_acceleration_default
		f1000 movement_acceleration_air
		f1000 movement_acceleration_fast
		f1000 movement_speed_walk
		f1000 movement_speed_crouch
		f1000 movement_speed_fast
		f1000 movement_speed_climb
		f1000 movement_speed_jump
		f1000 movement_liquid_fluidity
		f1000 movement_liquid_fluidity_smooth
		f1000 movement_liquid_sink
		f1000 movement_gravity
	*/

	TOCLIENT_SPAWN_PARTICLE = 0x46,
	/*
		u16 command
		v3f1000 pos
		v3f1000 velocity
		v3f1000 acceleration
		f1000 expirationtime
		f1000 size
		u8 bool collisiondetection
		u8 bool vertical
		u32 len
		u8[len] texture
	*/

	TOCLIENT_ADD_PARTICLESPAWNER = 0x47,
	/*
		u16 command
		u16 amount
		f1000 spawntime
		v3f1000 minpos
		v3f1000 maxpos
		v3f1000 minvel
		v3f1000 maxvel
		v3f1000 minacc
		v3f1000 maxacc
		f1000 minexptime
		f1000 maxexptime
		f1000 minsize
		f1000 maxsize
		u8 bool collisiondetection
		u8 bool vertical
		u32 len
		u8[len] texture
		u32 id
	*/

	TOCLIENT_DELETE_PARTICLESPAWNER = 0x48,
	/*
		u16 command
		u32 id
	*/

	TOCLIENT_HUDADD = 0x49,
	/*
		u16 command
		u32 id
		u8 type
		v2f1000 pos
		u32 len
		u8[len] name
		v2f1000 scale
		u32 len2
		u8[len2] text
		u32 number
		u32 item
		u32 dir
		v2f1000 align
		v2f1000 offset
		v3f1000 world_pos
		v2s32 size
	*/

	TOCLIENT_HUDRM = 0x4a,
	/*
		u16 command
		u32 id
	*/

	TOCLIENT_HUDCHANGE = 0x4b,
	/*
		u16 command
		u32 id
		u8 stat
		[v2f1000 data |
		 u32 len
		 u8[len] data |
		 u32 data]
	*/

	TOCLIENT_HUD_SET_FLAGS = 0x4c,
	/*
		u16 command
		u32 flags
		u32 mask
	*/

	TOCLIENT_HUD_SET_PARAM = 0x4d,
	/*
		u16 command
		u16 param
		u16 len
		u8[len] value
	*/

	TOCLIENT_BREATH = 0x4e,
	/*
		u16 command
		u16 breath
	*/

	TOCLIENT_SET_SKY = 0x4f,
	/*
		u16 command
		u8[4] color (ARGB)
		u8 len
		u8[len] type
		u16 count
		foreach count:
			u8 len
			u8[len] param
	*/

	TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO = 0x50,
	/*
		u16 command
		u8 do_override (boolean)
		u16 day-night ratio 0...65535
	*/

	TOCLIENT_LOCAL_PLAYER_ANIMATIONS = 0x51,
	/*
		u16 command
		v2s32 stand/idle
		v2s32 walk
		v2s32 dig
		v2s32 walk+dig
		f1000 frame_speed
	*/

	TOCLIENT_EYE_OFFSET = 0x52,
	/*
		u16 command
		v3f1000 first
		v3f1000 third
	*/
};

enum ToServerCommand
{
	TOSERVER_INIT=0x10,
	/*
		Sent first after connected.

		[0] u16 TOSERVER_INIT
		[2] u8 SER_FMT_VER_HIGHEST_READ
		[3] u8[20] player_name
		[23] u8[28] password (new in some version)
		[51] u16 minimum supported network protocol version (added sometime)
		[53] u16 maximum supported network protocol version (added later than the previous one)
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
		[2+12+12+4+4] u32 keyPressed
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

	TOSERVER_CLICK_OBJECT = 0x27, // Obsolete
	/*
		length: 13
		[0] u16 command
		[2] u8 button (0=left, 1=right)
		[3] v3s16 blockpos
		[9] s16 id
		[11] u16 item
	*/

	TOSERVER_GROUND_ACTION = 0x28, // Obsolete
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

	// (oops, there is some gap here)

	TOSERVER_SIGNTEXT = 0x30, // Old signs, obsolete
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

	TOSERVER_SIGNNODETEXT = 0x33, // obsolete
	/*
		u16 command
		v3s16 p
		u16 textlen
		textdata
	*/

	TOSERVER_CLICK_ACTIVEOBJECT = 0x34, // Obsolete
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

	TOSERVER_PASSWORD=0x36,
	/*
		Sent to change password.

		[0] u16 TOSERVER_PASSWORD
		[2] u8[28] old password
		[30] u8[28] new password
	*/

	TOSERVER_PLAYERITEM=0x37,
	/*
		Sent to change selected item.

		[0] u16 TOSERVER_PLAYERITEM
		[2] u16 item
	*/

	TOSERVER_RESPAWN=0x38,
	/*
		u16 TOSERVER_RESPAWN
	*/

	TOSERVER_INTERACT = 0x39,
	/*
		[0] u16 command
		[2] u8 action
		[3] u16 item
		[5] u32 length of the next item
		[9] serialized PointedThing
		actions:
		0: start digging (from undersurface) or use
		1: stop digging (all parameters ignored)
		2: digging completed
		3: place block or item (to abovesurface)
		4: use item

		(Obsoletes TOSERVER_GROUND_ACTION and TOSERVER_CLICK_ACTIVEOBJECT.)
	*/

	TOSERVER_REMOVED_SOUNDS = 0x3a,
	/*
		u16 command
		u16 len
		s32[len] sound_id
	*/

	TOSERVER_NODEMETA_FIELDS = 0x3b,
	/*
		u16 command
		v3s16 p
		u16 len
		u8[len] form name (reserved for future use)
		u16 number of fields
		for each field:
			u16 len
			u8[len] field name
			u32 len
			u8[len] field value
	*/

	TOSERVER_INVENTORY_FIELDS = 0x3c,
	/*
		u16 command
		u16 len
		u8[len] form name (reserved for future use)
		u16 number of fields
		for each field:
			u16 len
			u8[len] field name
			u32 len
			u8[len] field value
	*/

	TOSERVER_REQUEST_MEDIA = 0x40,
	/*
		u16 command
		u16 number of files requested
		for each file {
			u16 length of name
			string name
		}
	 */

	TOSERVER_RECEIVED_MEDIA = 0x41,
	/*
		u16 command
	*/

	TOSERVER_BREATH = 0x42,
	/*
		u16 command
		u16 breath
	*/

	TOSERVER_CLIENT_READY = 0x43,
	/*
		u8 major
		u8 minor
		u8 patch
		u8 reserved
		u16 len
		u8[len] full_version_string
	*/
};

#endif


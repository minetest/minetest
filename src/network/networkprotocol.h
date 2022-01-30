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
		Obsolete TOCLIENT_PLAYERITEM
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
		AO_CMD_SET_ANIMATION
		AO_CMD_SET_BONE_POSITION
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
		AO_CMD_SET_PHYSICS_OVERRIDE
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
		Obsolete TOSERVER_RECEIVED_MEDIA
		Server: Stop using TOSERVER_CLIENT_READY
	PROTOCOL_VERSION 24:
		ContentFeatures version 7
		ContentFeatures: change number of special tiles to 6 (CF_SPECIAL_COUNT)
	PROTOCOL_VERSION 25:
		Rename TOCLIENT_ACCESS_DENIED to TOCLIENT_ACCESS_DENIED_LEGAGY
		Rename TOCLIENT_DELETE_PARTICLESPAWNER to
			TOCLIENT_DELETE_PARTICLESPAWNER_LEGACY
		Rename TOSERVER_PASSWORD to TOSERVER_PASSWORD_LEGACY
		Rename TOSERVER_INIT to TOSERVER_INIT_LEGACY
		Rename TOCLIENT_INIT to TOCLIENT_INIT_LEGACY
		Add TOCLIENT_ACCESS_DENIED new opcode (0x0A), using error codes
			for standard error, keeping customisation possible. This
			permit translation
		Add TOCLIENT_DELETE_PARTICLESPAWNER (0x53), fixing the u16 read and
			reading u32
		Add new opcode TOSERVER_INIT for client presentation to server
		Add new opcodes TOSERVER_FIRST_SRP, TOSERVER_SRP_BYTES_A,
			TOSERVER_SRP_BYTES_M, TOCLIENT_SRP_BYTES_S_B
			for the three supported auth mechanisms around srp
		Add new opcodes TOCLIENT_ACCEPT_SUDO_MODE and TOCLIENT_DENY_SUDO_MODE
			for sudo mode handling (auth mech generic way of changing password).
		Add TOCLIENT_HELLO for presenting server to client after client
			presentation
		Add TOCLIENT_AUTH_ACCEPT to accept connection from client
		Rename GENERIC_CMD_SET_ATTACHMENT to AO_CMD_ATTACH_TO
	PROTOCOL_VERSION 26:
		Add TileDef tileable_horizontal, tileable_vertical flags
	PROTOCOL_VERSION 27:
		backface_culling: backwards compatibility for playing with
		newer client on pre-27 servers.
		Add nodedef v3 - connected nodeboxes
	PROTOCOL_VERSION 28:
		CPT2_MESHOPTIONS
	PROTOCOL_VERSION 29:
		Server doesn't accept TOSERVER_BREATH anymore
		serialization of TileAnimation params changed
		TAT_SHEET_2D
		Removed client-sided chat perdiction
	PROTOCOL VERSION 30:
		New ContentFeatures serialization version
		Add node and tile color and palette
		Fix plantlike visual_scale being applied squared and add compatibility
			with pre-30 clients by sending sqrt(visual_scale)
	PROTOCOL VERSION 31:
		Add tile overlay
		Stop sending TOSERVER_CLIENT_READY
	PROTOCOL VERSION 32:
		Add fading sounds
	PROTOCOL VERSION 33:
		Add TOCLIENT_UPDATE_PLAYER_LIST and send the player list to the client,
			instead of guessing based on the active object list.
	PROTOCOL VERSION 34:
		Add sound pitch
	PROTOCOL VERSION 35:
 		Rename TOCLIENT_CHAT_MESSAGE to TOCLIENT_CHAT_MESSAGE_OLD (0x30)
 		Add TOCLIENT_CHAT_MESSAGE (0x2F)
 			This chat message is a signalisation message containing various
			informations:
 			* timestamp
 			* sender
 			* type (RAW, NORMAL, ANNOUNCE, SYSTEM)
 			* content
		Add TOCLIENT_CSM_RESTRICTION_FLAGS to define which CSM features should be
			limited
		Add settable player collisionbox. Breaks compatibility with older
			clients as a 1-node vertical offset has been removed from player's
			position
		Add settable player stepheight using existing object property.
			Breaks compatibility with older clients.
	PROTOCOL VERSION 36:
		Backwards compatibility drop
		Add 'can_zoom' to player object properties
		Add glow to object properties
		Change TileDef serialization format.
		Add world-aligned tiles.
		Mod channels
		Raise ObjectProperties version to 3 for removing 'can_zoom' and adding
			'zoom_fov'.
		Nodebox version 5
		Add disconnected nodeboxes
		Add TOCLIENT_FORMSPEC_PREPEND
	PROTOCOL VERSION 37:
		Redo detached inventory sending
		Add TOCLIENT_NODEMETA_CHANGED
		New network float format
		ContentFeatures version 13
		Add full Euler rotations instead of just yaw
		Add TOCLIENT_PLAYER_SPEED
	PROTOCOL VERSION 38:
		Incremental inventory sending mode
		Unknown inventory serialization fields no longer throw an error
		Mod-specific formspec version
		Player FOV override API
		"ephemeral" added to TOCLIENT_PLAY_SOUND
	PROTOCOL VERSION 39:
		Updated set_sky packet
		Adds new sun, moon and stars packets
		Minimap modes
	PROTOCOL VERSION 40:
		TOCLIENT_MEDIA_PUSH changed, TOSERVER_HAVE_MEDIA added
*/

#define LATEST_PROTOCOL_VERSION 40
#define LATEST_PROTOCOL_VERSION_STRING TOSTRING(LATEST_PROTOCOL_VERSION)

// Server's supported network protocol range
#define SERVER_PROTOCOL_VERSION_MIN 37
#define SERVER_PROTOCOL_VERSION_MAX LATEST_PROTOCOL_VERSION

// Client's supported network protocol range
// The minimal version depends on whether
// send_pre_v25_init is enabled or not
#define CLIENT_PROTOCOL_VERSION_MIN 37
#define CLIENT_PROTOCOL_VERSION_MAX LATEST_PROTOCOL_VERSION

// Constant that differentiates the protocol from random data and other protocols
#define PROTOCOL_ID 0x4f457403

#define PASSWORD_SIZE 28       // Maximum password length. Allows for
                               // base64-encoded SHA-1 (27+\0).

// See also: Formspec Version History in doc/lua_api.txt
#define FORMSPEC_API_VERSION 5

#define TEXTURENAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-"

typedef u16 session_t;

enum ToClientCommand
{
	TOCLIENT_HELLO = 0x02,
	/*
		Sent after TOSERVER_INIT.

		u8 deployed serialisation version
		u16 deployed network compression mode
		u16 deployed protocol version
		u32 supported auth methods
		std::string username that should be used for legacy hash (for proper casing)
	*/
	TOCLIENT_AUTH_ACCEPT = 0x03,
	/*
		Message from server to accept auth.

		v3s16 player's position + v3f(0,BS/2,0) floatToInt'd
		u64 map seed
		f1000 recommended send interval
		u32 : supported auth methods for sudo mode
		      (where the user can change their password)
	*/
	TOCLIENT_ACCEPT_SUDO_MODE = 0x04,
	/*
		Sent to client to show it is in sudo mode now.
	*/
	TOCLIENT_DENY_SUDO_MODE = 0x05,
	/*
		Signals client that sudo mode auth failed.
	*/
	TOCLIENT_ACCESS_DENIED = 0x0A,
	/*
		u8 reason
		std::string custom reason (if needed, otherwise "")
		u8 (bool) reconnect
	*/

	TOCLIENT_INIT_LEGACY = 0x10, // Obsolete

	TOCLIENT_BLOCKDATA = 0x20, //TODO: Multiple blocks
	TOCLIENT_ADDNODE = 0x21,
	/*
		v3s16 position
		serialized mapnode
		u8 keep_metadata // Added in protocol version 22
	*/
	TOCLIENT_REMOVENODE = 0x22,

	TOCLIENT_PLAYERPOS = 0x23, // Obsolete
	TOCLIENT_PLAYERINFO = 0x24, // Obsolete
	TOCLIENT_OPT_BLOCK_NOT_FOUND = 0x25, // Obsolete
	TOCLIENT_SECTORMETA = 0x26, // Obsolete

	TOCLIENT_INVENTORY = 0x27,
	/*
		[0] u16 command
		[2] serialized inventory
	*/

	TOCLIENT_OBJECTDATA = 0x28, // Obsolete

	TOCLIENT_TIME_OF_DAY = 0x29,
	/*
		u16 time (0-23999)
		Added in a later version:
		f1000 time_speed
	*/

	TOCLIENT_CSM_RESTRICTION_FLAGS = 0x2A,
	/*
		u32 CSMRestrictionFlags byteflag
	 */

	TOCLIENT_PLAYER_SPEED = 0x2B,
	/*
		v3f added_vel
	 */

	TOCLIENT_MEDIA_PUSH = 0x2C,
	/*
		std::string raw_hash
		std::string filename
		u32 callback_token
		bool should_be_cached
	*/

	// (oops, there is some gap here)

	TOCLIENT_CHAT_MESSAGE = 0x2F,
	/*
		u8 version
		u8 message_type
		u16 sendername length
		wstring sendername
		u16 length
		wstring message
	*/

	TOCLIENT_CHAT_MESSAGE_OLD = 0x30, // Obsolete

	TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD = 0x31,
	/*
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
		for all objects
		{
			u16 id
			u16 message length
			string message
		}
	*/

	TOCLIENT_HP = 0x33,
	/*
		u8 hp
	*/

	TOCLIENT_MOVE_PLAYER = 0x34,
	/*
		v3f1000 player position
		f1000 player pitch
		f1000 player yaw
	*/

	TOCLIENT_ACCESS_DENIED_LEGACY = 0x35,
	/*
		u16 reason_length
		wstring reason
	*/

	TOCLIENT_FOV = 0x36,
	/*
		Sends an FOV override/multiplier to client.

		f32 fov
		bool is_multiplier
		f32 transition_time
	*/

	TOCLIENT_DEATHSCREEN = 0x37,
	/*
		u8 bool set camera point target
		v3f1000 camera point target (to point the death cause or whatever)
	*/

	TOCLIENT_MEDIA = 0x38,
	/*
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
		u32 length of the next item
		serialized ToolDefManager
	*/

	TOCLIENT_NODEDEF = 0x3a,
	/*
		u32 length of the next item
		serialized NodeDefManager
	*/

	TOCLIENT_CRAFTITEMDEF = 0x3b,
	/*
		u32 length of the next item
		serialized CraftiItemDefManager
	*/

	TOCLIENT_ANNOUNCE_MEDIA = 0x3c,

	/*
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
		u32 length of next item
		serialized ItemDefManager
	*/

	TOCLIENT_PLAY_SOUND = 0x3f,
	/*
		s32 sound_id
		u16 len
		u8[len] sound name
		s32 gain*1000
		u8 type (0=local, 1=positional, 2=object)
		s32[3] pos_nodes*10000
		u16 object_id
		u8 loop (bool)
		u8 ephemeral (bool)
	*/

	TOCLIENT_STOP_SOUND = 0x40,
	/*
		s32 sound_id
	*/

	TOCLIENT_PRIVILEGES = 0x41,
	/*
		u16 number of privileges
		for each privilege
			u16 len
			u8[len] privilege
	*/

	TOCLIENT_INVENTORY_FORMSPEC = 0x42,
	/*
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
		v3f1000 pos
		v3f1000 velocity
		v3f1000 acceleration
		f1000 expirationtime
		f1000 size
		u8 bool collisiondetection
		u32 len
		u8[len] texture
		u8 bool vertical
		u8 collision_removal
		TileAnimation animation
		u8 glow
		u8 object_collision
	*/

	TOCLIENT_ADD_PARTICLESPAWNER = 0x47,
	/*
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
		u32 len
		u8[len] texture
		u8 bool vertical
		u8 collision_removal
		u32 id
		TileAnimation animation
		u8 glow
		u8 object_collision
	*/

	TOCLIENT_DELETE_PARTICLESPAWNER_LEGACY = 0x48, // Obsolete

	TOCLIENT_HUDADD = 0x49,
	/*
		u32 id
		u8 type
		v2f1000 pos
		u16 len
		u8[len] name
		v2f1000 scale
		u16 len2
		u8[len2] text
		u32 number
		u32 item
		u32 dir
		v2f1000 align
		v2f1000 offset
		v3f1000 world_pos
		v2s32 size
		s16 z_index
		u16 len3
		u8[len3] text2
	*/

	TOCLIENT_HUDRM = 0x4a,
	/*
		u32 id
	*/

	TOCLIENT_HUDCHANGE = 0x4b,
	/*
		u32 id
		u8 stat
		[v2f1000 data |
		 u32 len
		 u8[len] data |
		 u32 data]
	*/

	TOCLIENT_HUD_SET_FLAGS = 0x4c,
	/*
		u32 flags
		u32 mask
	*/

	TOCLIENT_HUD_SET_PARAM = 0x4d,
	/*
		u16 param
		u16 len
		u8[len] value
	*/

	TOCLIENT_BREATH = 0x4e,
	/*
		u16 breath
	*/

	TOCLIENT_SET_SKY = 0x4f,
	/*
		Protocol 38:
		u8[4] base_color (ARGB)
		u8 len
		u8[len] type
		u16 count
		foreach count:
			u8 len
			u8[len] param
		u8 clouds (boolean)

		Protocol 39:
		u8[4] bgcolor (ARGB)
		std::string type
		int texture_count
		std::string[6] param
		bool clouds
		bool bgcolor_fog
		u8[4] day_sky (ARGB)
		u8[4] day_horizon (ARGB)
		u8[4] dawn_sky (ARGB)
		u8[4] dawn_horizon (ARGB)
		u8[4] night_sky (ARGB)
		u8[4] night_horizon (ARGB)
		u8[4] indoors (ARGB)
		u8[4] fog_sun_tint (ARGB)
		u8[4] fog_moon_tint (ARGB)
		std::string fog_tint_type
	*/

	TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO = 0x50,
	/*
		u8 do_override (boolean)
		u16 day-night ratio 0...65535
	*/

	TOCLIENT_LOCAL_PLAYER_ANIMATIONS = 0x51,
	/*
		v2s32 stand/idle
		v2s32 walk
		v2s32 dig
		v2s32 walk+dig
		f1000 frame_speed
	*/

	TOCLIENT_EYE_OFFSET = 0x52,
	/*
		v3f1000 first
		v3f1000 third
	*/

	TOCLIENT_DELETE_PARTICLESPAWNER = 0x53,
	/*
		u32 id
	*/

	TOCLIENT_CLOUD_PARAMS = 0x54,
	/*
		f1000 density
		u8[4] color_diffuse (ARGB)
		u8[4] color_ambient (ARGB)
		f1000 height
		f1000 thickness
		v2f1000 speed
	*/

	TOCLIENT_FADE_SOUND = 0x55,
	/*
		s32 sound_id
		float step
		float gain
	*/
	TOCLIENT_UPDATE_PLAYER_LIST = 0x56,
	/*
	 	u8 type
	 	u16 number of players
		for each player
			u16 len
			u8[len] player name
	*/

	TOCLIENT_MODCHANNEL_MSG = 0x57,
	/*
		u16 channel name length
	 	std::string channel name
	 	u16 channel name sender
	 	std::string channel name
	 	u16 message length
	 	std::string message
	*/

	TOCLIENT_MODCHANNEL_SIGNAL = 0x58,
	/*
		u8 signal id
	 	u16 channel name length
	 	std::string channel name
	*/

	TOCLIENT_NODEMETA_CHANGED = 0x59,
	/*
		serialized and compressed node metadata
	*/

	TOCLIENT_SET_SUN = 0x5a,
	/*
		bool visible
		std::string texture
		std::string tonemap
		std::string sunrise
		f32 scale
	*/

	TOCLIENT_SET_MOON = 0x5b,
	/*
		bool visible
		std::string texture
		std::string tonemap
		f32 scale
	*/

	TOCLIENT_SET_STARS = 0x5c,
	/*
		bool visible
		u32 count
		u8[4] starcolor (ARGB)
		f32 scale
	*/

	TOCLIENT_SRP_BYTES_S_B = 0x60,
	/*
		Belonging to AUTH_MECHANISM_SRP.

		std::string bytes_s
		std::string bytes_B
	*/

	TOCLIENT_FORMSPEC_PREPEND = 0x61,
	/*
		u16 len
		u8[len] formspec
	*/

	TOCLIENT_MINIMAP_MODES = 0x62,
	/*
		u16 count // modes
		u16 mode  // wanted current mode index after change
		for each mode
			u16 type
			std::string label
			u16 size
			std::string extra
	*/

	TOCLIENT_NUM_MSG_TYPES = 0x63,
};

enum ToServerCommand
{
	TOSERVER_INIT = 0x02,
	/*
		Sent first after connected.

		u8 serialisation version (=SER_FMT_VER_HIGHEST_READ)
		u16 supported network compression modes
		u16 minimum supported network protocol version
		u16 maximum supported network protocol version
		std::string player name
	*/

	TOSERVER_INIT_LEGACY = 0x10, // Obsolete

	TOSERVER_INIT2 = 0x11,
	/*
		Sent as an ACK for TOCLIENT_INIT.
		After this, the server can send data.

		[0] u16 TOSERVER_INIT2
	*/

	TOSERVER_MODCHANNEL_JOIN = 0x17,
	/*
		u16 channel name length
	 	std::string channel name
	 */

	TOSERVER_MODCHANNEL_LEAVE = 0x18,
	/*
		u16 channel name length
	 	std::string channel name
	 */

	TOSERVER_MODCHANNEL_MSG = 0x19,
	/*
		u16 channel name length
	 	std::string channel name
	 	u16 message length
	 	std::string message
	 */

	TOSERVER_GETBLOCK = 0x20, // Obsolete
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
		[2+12+12+4+4+1] u8 fov*80
		[2+12+12+4+4+4+1] u8 ceil(wanted_range / MAP_BLOCKSIZE)
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
	TOSERVER_CLICK_OBJECT = 0x27, // Obsolete
	TOSERVER_GROUND_ACTION = 0x28, // Obsolete
	TOSERVER_RELEASE = 0x29, // Obsolete
	TOSERVER_SIGNTEXT = 0x30, // Obsolete

	TOSERVER_INVENTORY_ACTION = 0x31,
	/*
		See InventoryAction in inventorymanager.h
	*/

	TOSERVER_CHAT_MESSAGE = 0x32,
	/*
		u16 length
		wstring message
	*/

	TOSERVER_SIGNNODETEXT = 0x33, // Obsolete
	TOSERVER_CLICK_ACTIVEOBJECT = 0x34, // Obsolete

	TOSERVER_DAMAGE = 0x35,
	/*
		u8 amount
	*/

	TOSERVER_PASSWORD_LEGACY = 0x36, // Obsolete

	TOSERVER_PLAYERITEM = 0x37,
	/*
		Sent to change selected item.

		[0] u16 TOSERVER_PLAYERITEM
		[2] u16 item
	*/

	TOSERVER_RESPAWN = 0x38,
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
	*/

	TOSERVER_REMOVED_SOUNDS = 0x3a,
	/*
		u16 len
		s32[len] sound_id
	*/

	TOSERVER_NODEMETA_FIELDS = 0x3b,
	/*
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
		u16 number of files requested
		for each file {
			u16 length of name
			string name
		}
	*/

	TOSERVER_HAVE_MEDIA = 0x41,
	/*
		u8 number of callback tokens
		for each:
			u32 token
	*/

	TOSERVER_BREATH = 0x42, // Obsolete

	TOSERVER_CLIENT_READY = 0x43,
	/*
		u8 major
		u8 minor
		u8 patch
		u8 reserved
		u16 len
		u8[len] full_version_string
	*/

	TOSERVER_FIRST_SRP = 0x50,
	/*
		Belonging to AUTH_MECHANISM_FIRST_SRP.

		std::string srp salt
		std::string srp verification key
		u8 is_empty (=1 if password is empty, 0 otherwise)
	*/

	TOSERVER_SRP_BYTES_A = 0x51,
	/*
		Belonging to AUTH_MECHANISM_SRP,
			depending on current_login_based_on.

		std::string bytes_A
		u8 current_login_based_on : on which version of the password's
		                            hash this login is based on (0 legacy hash,
		                            or 1 directly the password)
	*/

	TOSERVER_SRP_BYTES_M = 0x52,
	/*
		Belonging to AUTH_MECHANISM_SRP.

		std::string bytes_M
	*/

	TOSERVER_NUM_MSG_TYPES = 0x53,
};

enum AuthMechanism
{
	// reserved
	AUTH_MECHANISM_NONE = 0,

	// SRP based on the legacy hash
	AUTH_MECHANISM_LEGACY_PASSWORD = 1 << 0,

	// SRP based on the srp verification key
	AUTH_MECHANISM_SRP = 1 << 1,

	// Establishes a srp verification key, for first login and password changing
	AUTH_MECHANISM_FIRST_SRP = 1 << 2,
};

enum AccessDeniedCode {
	SERVER_ACCESSDENIED_WRONG_PASSWORD,
	SERVER_ACCESSDENIED_UNEXPECTED_DATA,
	SERVER_ACCESSDENIED_SINGLEPLAYER,
	SERVER_ACCESSDENIED_WRONG_VERSION,
	SERVER_ACCESSDENIED_WRONG_CHARS_IN_NAME,
	SERVER_ACCESSDENIED_WRONG_NAME,
	SERVER_ACCESSDENIED_TOO_MANY_USERS,
	SERVER_ACCESSDENIED_EMPTY_PASSWORD,
	SERVER_ACCESSDENIED_ALREADY_CONNECTED,
	SERVER_ACCESSDENIED_SERVER_FAIL,
	SERVER_ACCESSDENIED_CUSTOM_STRING,
	SERVER_ACCESSDENIED_SHUTDOWN,
	SERVER_ACCESSDENIED_CRASH,
	SERVER_ACCESSDENIED_MAX,
};

enum NetProtoCompressionMode {
	NETPROTO_COMPRESSION_NONE = 0,
};

const static std::string accessDeniedStrings[SERVER_ACCESSDENIED_MAX] = {
	"Invalid password",
	"Your client sent something the server didn't expect.  Try reconnecting or updating your client",
	"The server is running in simple singleplayer mode.  You cannot connect.",
	"Your client's version is not supported.\nPlease contact server administrator.",
	"Player name contains disallowed characters.",
	"Player name not allowed.",
	"Too many users.",
	"Empty passwords are disallowed.  Set a password and try again.",
	"Another client is connected with this name.  If your client closed unexpectedly, try again in a minute.",
	"Server authentication failed.  This is likely a server error.",
	"",
	"Server shutting down.",
	"This server has experienced an internal error. You will now be disconnected."
};

enum PlayerListModifer : u8
{
	PLAYER_LIST_INIT,
	PLAYER_LIST_ADD,
	PLAYER_LIST_REMOVE,
};

enum CSMRestrictionFlags : u64 {
	CSM_RF_NONE = 0x00000000,
	// Until server-sent CSM and verifying of builtin are complete,
	// 'CSM_RF_LOAD_CLIENT_MODS' also disables loading 'builtin'.
	// When those are complete, this should return to only being a restriction on the
	// loading of client mods.
	CSM_RF_LOAD_CLIENT_MODS = 0x00000001, // Don't load client-provided mods or 'builtin'
	CSM_RF_CHAT_MESSAGES = 0x00000002,    // Disable chat message sending from CSM
	CSM_RF_READ_ITEMDEFS = 0x00000004,    // Disable itemdef lookups
	CSM_RF_READ_NODEDEFS = 0x00000008,    // Disable nodedef lookups
	CSM_RF_LOOKUP_NODES = 0x00000010,     // Limit node lookups
	CSM_RF_READ_PLAYERINFO = 0x00000020,  // Disable player info lookups
	CSM_RF_ALL = 0xFFFFFFFF,
};

enum InteractAction : u8
{
	INTERACT_START_DIGGING,     // 0: start digging (from undersurface) or use
	INTERACT_STOP_DIGGING,      // 1: stop digging (all parameters ignored)
	INTERACT_DIGGING_COMPLETED, // 2: digging completed
	INTERACT_PLACE,             // 3: place block or item (to abovesurface)
	INTERACT_USE,               // 4: use item
	INTERACT_ACTIVATE           // 5: rightclick air ("activate")
};

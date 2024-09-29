// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "networkprotocol.h"


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
	PROTOCOL VERSION 41:
		Added new particlespawner parameters
		[scheduled bump for 5.6.0]
	PROTOCOL VERSION 42:
		TOSERVER_UPDATE_CLIENT_INFO added
		new fields for TOCLIENT_SET_LIGHTING and TOCLIENT_SET_SKY
		Send forgotten TweenedParameter properties
		[scheduled bump for 5.7.0]
	PROTOCOL VERSION 43:
		"start_time" added to TOCLIENT_PLAY_SOUND
		place_param2 type change u8 -> optional<u8>
		[scheduled bump for 5.8.0]
	PROTOCOL VERSION 44:
		AO_CMD_SET_BONE_POSITION extended
		Add TOCLIENT_MOVE_PLAYER_REL
		Move default minimap from client-side C++ to server-side builtin Lua
		[scheduled bump for 5.9.0]
	PROTOCOL VERSION 45:
		Minimap HUD element supports negative size values as percentages
		[bump for 5.9.1]
	PROTOCOL VERSION 46:
		Move default hotbar from client-side C++ to server-side builtin Lua
		Add shadow tint to Lighting packets
		Add shadow color to CloudParam packets
		Move death screen to server and make it a regular formspec
			The server no longer triggers the hardcoded client-side death
			formspec, but the client still supports it for compatibility with
			old servers.
		Rename TOCLIENT_DEATHSCREEN to TOCLIENT_DEATHSCREEN_LEGACY
		Rename TOSERVER_RESPAWN to TOSERVER_RESPAWN_LEGACY
		[scheduled bump for 5.10.0]
*/

const u16 LATEST_PROTOCOL_VERSION = 46;

// See also formspec [Version History] in doc/lua_api.md
const u16 FORMSPEC_API_VERSION = 7;

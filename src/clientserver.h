#ifndef CLIENTSERVER_HEADER
#define CLIENTSERVER_HEADER

#define PROTOCOL_ID 0x4f457403

enum ToClientCommand
{
	TOCLIENT_INIT=0x10,
	/*
		Server's reply to TOSERVER_INIT.
		Sent second after connected.

		[0] u16 TOSERVER_INIT
		[2] u8 deployed version
		[3] v3s16 player's position + v3f(0,BS/2,0) floatToInt'd
	*/

	TOCLIENT_BLOCKDATA=0x20, //TODO: Multiple blocks
	TOCLIENT_ADDNODE,
	TOCLIENT_REMOVENODE,
	
	TOCLIENT_PLAYERPOS,
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

	TOCLIENT_PLAYERINFO,
	/*
		[0] u16 command
		// Followed by an arbitary number of these:
		// Number is determined from packet length.
		[N] u16 peer_id
		[N] char[20] name
	*/
	
	TOCLIENT_OPT_BLOCK_NOT_FOUND, // Not used

	TOCLIENT_SECTORMETA,
	/*
		[0] u16 command
		[2] u8 sector count
		[3...] v2s16 pos + sector metadata
	*/

	TOCLIENT_INVENTORY,
	/*
		[0] u16 command
		[2] serialized inventory
	*/
	
	TOCLIENT_OBJECTDATA,
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
};

enum ToServerCommand
{
	TOSERVER_INIT=0x10,
	/*
		Sent first after connected.

		[0] u16 TOSERVER_INIT
		[2] u8 SER_FMT_VER_HIGHEST
		[3] u8[20] player_name
	*/

	TOSERVER_INIT2,
	/*
		Sent as an ACK for TOCLIENT_INIT.
		After this, the server can send data.

		[0] u16 TOSERVER_INIT2
	*/

	TOSERVER_GETBLOCK=0x20, // Not used
	TOSERVER_ADDNODE, // Not used
	TOSERVER_REMOVENODE, // deprecated

	TOSERVER_PLAYERPOS,
	/*
		[0] u16 command
		[2] v3s32 position*100
		[2+12] v3s32 speed*100
		[2+12+12] s32 pitch*100
		[2+12+12+4] s32 yaw*100
	*/

	TOSERVER_GOTBLOCKS,
	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	TOSERVER_DELETEDBLOCKS,
	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	TOSERVER_ADDNODE_FROM_INVENTORY, // deprecated
	/*
		[0] u16 command
		[2] v3s16 pos
		[8] u16 i
	*/

	TOSERVER_CLICK_OBJECT,
	/*
		length: 13
		[0] u16 command
		[2] u8 button (0=left, 1=right)
		[3] v3s16 blockpos
		[9] s16 id
		[11] u16 item
	*/

	TOSERVER_CLICK_GROUND,
	/*
		length: 17
		[0] u16 command
		[2] u8 button (0=left, 1=right)
		[3] v3s16 nodepos_undersurface
		[9] v3s16 nodepos_abovesurface
		[15] u16 item
	*/
	
	TOSERVER_RELEASE,
	/*
		length: 3
		[0] u16 command
		[2] u8 button
	*/

	TOSERVER_SIGNTEXT,
	/*
		u16 command
		v3s16 blockpos
		s16 id
		u16 textlen
		textdata
	*/
};

// Flags for TOSERVER_GETBLOCK
#define TOSERVER_GETBLOCK_FLAG_OPTIONAL (1<<0)

#endif


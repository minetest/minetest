#include "clientserver.h"
#include "util/serialize.h"
#include "mapblock.h"
#include "inventory.h"

namespace protocol {

SharedBuffer<u8> create_TOCLIENT_INIT(
		u16 net_proto_version, // Always
		u8 deployed_block_version,
		const v3s16 &player_pos,
		const u64 &map_seed,
		const float recommended_send_interval
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_INIT);
	writeU8(os, deployed_block_version);
	writeV3S16(os, player_pos);
	writeU64(os, map_seed);
	writeF1000(os, recommended_send_interval);

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_BLOCKDATA(
		u16 net_proto_version, // Always
		u8 block_format_version,
		v3s16 position,
		const MapBlock *block
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_BLOCKDATA);
	writeV3S16(os, position);
	block->serialize(os, block_format_version, false);
	block->serializeNetworkSpecific(os, net_proto_version);

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_ADDNODE(
		u16 net_proto_version, // Always
		u8 block_format_version,
		const v3s16 &p,
		const MapNode &n,
		bool keep_metadata
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_ADDNODE);
	writeV3S16(os, p);
	SharedBuffer<u8> n_data(MapNode::serializedLength(block_format_version));
	n.serialize(&n_data[0], block_format_version);
	writeU8(os, keep_metadata);

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_REMOVENODE(
		u16 net_proto_version, // Always
		const v3s16 &p
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_REMOVENODE);
	writeV3S16(os, p);

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_INVENTORY(
		u16 net_proto_version, // Always
		const Inventory* inventory
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_INVENTORY);
	inventory->serialize(os);

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_TIME_OF_DAY(
		u16 net_proto_version, // Always
		u16 time,
		float time_speed
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_TIME_OF_DAY);
	writeU16(os, time);
	writeF1000(os, time_speed);

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_CHAT_MESSAGE(
		u16 net_proto_version, // Always
		std::wstring message
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_CHAT_MESSAGE);
	writeU16(os, message.size());
	for(u32 i=0; i<message.size(); i++){
		u16 w = message[i];
		writeU16(os, w);
	}

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD(
		u16 net_proto_version, // Always
		const std::set<u16> &removed_objects,
		const std::vector<AddedObject> &added_objects
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD);
	writeU16(os, removed_objects.size());
	for(std::set<u16>::iterator
			i = removed_objects.begin();
			i != removed_objects.end(); ++i){
		writeU16(os, *i);
	}
	writeU16(os, added_objects.size());
	for(size_t i=0; i<added_objects.size(); i++){
		const AddedObject &ao = added_objects[i];
		writeU16(os, ao.id);
		writeU8(os, ao.type);
		os<<serializeLongString(ao.init_data);
	}

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_ACTIVE_OBJECT_MESSAGES(
		u16 net_proto_version, // Always
		const std::vector<ActiveObjectMessage> &messages
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_HP(
		u16 net_proto_version, // Always
		u8 hp
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_MOVE_PLAYER(
		u16 net_proto_version, // Always
		v3f p,
		float pitch,
		float yaw
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_ACCESS_DENIED(
		u16 net_proto_version, // Always
		std::wstring reason
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_DEATHSCREEN(
		u16 net_proto_version, // Always
		bool set_camera_point_target,
		v3f camera_point_target
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_MEDIA(
		u16 net_proto_version, // Always
		u16 num_bunches,
		u16 bunch_i,
		const std::list<SendableMedia> &files,
		std::string remote_media_url
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_NODEDEF(
		u16 net_proto_version, // Always
		INodeDefManager &ndef
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_ANNOUNCE_MEDIA(
		u16 net_proto_version, // Always
		const std::list<SendableMediaAnnouncement> &announcements
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_ITEMDEF(
		u16 net_proto_version, // Always
		IItemDefManager &idef
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_PLAY_SOUND(
		u16 net_proto_version, // Always
		s32 sound_id,
		const std::string &sound_name,
		float gain,
		u8 type,
		v3f pos_nodes,
		u16 object_id,
		bool loop
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_STOP_SOUND(
		u16 net_proto_version, // Always
		s32 sound_id
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_PRIVILEGES(
		u16 net_proto_version, // Always
		const std::set<std::string> privileges
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_INVENTORY_FORMSPEC(
		u16 net_proto_version, // Always
		const std::string &formspec
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_DETACHED_INVENTORY(
		u16 net_proto_version, // Always
		const std::string &name,
		const Inventory *inventory
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_SHOW_FORMSPEC(
		u16 net_proto_version, // Always
		const std::string &formspec,
		const std::string &formname
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_MOVEMENT(
		u16 net_proto_version, // Always
		float movement_acceleration_default,
		float movement_acceleration_air,
		float movement_acceleration_fast,
		float movement_speed_walk,
		float movement_speed_crouch,
		float movement_speed_fast,
		float movement_speed_climb,
		float movement_speed_jump,
		float movement_liquid_fluidity,
		float movement_liquid_fluidity_smooth,
		float movement_liquid_sink,
		float movement_gravity
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_SPAWN_PARTICLE(
		u16 net_proto_version, // Always
		v3f pos,
		v3f velocity,
		v3f acceleration,
		float expirationtime,
		float size,
		bool collision_detection,
		bool vertical,
		const std::string &texture
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_ADD_PARTICLESPAWNER(
		u16 net_proto_version, // Always
		u16 amount,
		float spawntime,
		v3f minpos,
		v3f maxpos,
		v3f minvel,
		v3f maxvel,
		v3f minacc,
		v3f maxacc,
		float minexptime,
		float maxexptime,
		float minsize,
		float maxsize,
		bool collisiondetection,
		bool vertical,
		const std::string &texture,
		u32 id
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_DELETE_PARTICLESPAWNER(
		u16 net_proto_version, // Always
		u32 id
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_HUDADD(
		u16 net_proto_version, // Always
		u16 command,
		u32 id,
		u8 type,
		v2f pos,
		const std::string &name,
		v2f scale,
		const std::string &text,
		u32 number,
		u32 item,
		u32 dir,
		v2f align,
		v2f offset
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_HUDRM(
		u16 net_proto_version, // Always
		u32 id
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_HUDCHANGE(
		u16 net_proto_version, // Always
		u32 id,
		u8 stat,
		void *value
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_HUD_SET_FLAGS(
		u16 net_proto_version, // Always
		u32 flags,
		u32 mask
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_HUD_SET_PARAM(
		u16 net_proto_version, // Always
		u16 param,
		const std::string &value
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_BREATH(
		u16 net_proto_version, // Always
		u16 breath
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_SET_SKY(
		u16 net_proto_version, // Always
		const video::SColor &color,
		const std::string &type,
		const std::vector<std::string> &params
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

SharedBuffer<u8> create_TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO(
		u16 net_proto_version, // Always
		bool do_override,
		u16 day_night_ratio
){
	std::ostringstream os(std::ios_base::binary);

	// TODO

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

} // protocol

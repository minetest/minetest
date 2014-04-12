#include "clientserver.h"
#include "util/serialize.h"
#include "mapblock.h"

namespace protocol {

SharedBuffer<u8> create_TOCLIENT_INIT(
		u16 net_proto_version,
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
		u16 net_proto_version,
		v3s16 position,
		const MapBlock *block,
		u8 block_format_version
){
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_BLOCKDATA);
	writeS16(os, position.X);
	writeS16(os, position.Y);
	writeS16(os, position.Z);
	block->serialize(os, block_format_version, false);
	block->serializeNetworkSpecific(os, net_proto_version);

	std::string s = os.str();
	return SharedBuffer<u8>((u8*)s.c_str(), s.size());
}

} // protocol

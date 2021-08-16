/*
Minetest
Copyright (C) 2020 sfan5 <sfan5@live.de>

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

#include "particles.h"
using namespace ParticleParamTypes;

v2f ParticleParamTypes::vectorBlend(float* f, const v2f& a, const v2f& b) {
	return v2f(
		numericalBlend(f[0], a.X, b.X),
		numericalBlend(f[1], a.Y, b.Y)
	);
}
v3f ParticleParamTypes::vectorBlend(float* f, const v3f& a, const v3f& b) {
	return v3f(
		numericalBlend(f[0], a.X, b.X),
		numericalBlend(f[1], a.Y, b.Y),
		numericalBlend(f[2], a.Z, b.Z)
	);
}

#define PARAM_DEF_SRZR(T, wr, rd) \
	void ParticleParamTypes::serializeParameterValue  (std::ostream& os, T  v) {wr(os,v);  } \
	void ParticleParamTypes::deSerializeParameterValue(std::istream& is, T& v) {v = rd(is);}

PARAM_DEF_SRZR(u8,  writeU8,    readU8);
PARAM_DEF_SRZR(u16, writeU16,   readU16);
PARAM_DEF_SRZR(u32, writeU32,   readU32);
PARAM_DEF_SRZR(f32, writeF32,   readF32);
PARAM_DEF_SRZR(v2f, writeV2F32, readV2F32);
PARAM_DEF_SRZR(v3f, writeV3F32, readV3F32);

#undef PARAM_DEF_SRZR

void ServerParticleTexture::serialize(std::ostream &os, u16 protocol_ver) const {
	u8 flags = 0;
	animated && (flags |= 1<<0);
	writeU8(os, flags);

	alpha.serialize(os);
	scale.serialize(os);
	os << serializeString32(string);

	if (animated)
		animation.serialize(os, protocol_ver);
}
void ServerParticleTexture::deSerialize(std::istream &is, u16 protocol_ver) {
	u8 flags = readU8(is);
	animated = (flags |= 1<<0) > 0;

	alpha.deSerialize(is);
	scale.deSerialize(is);
	string = deSerializeString32(is);

	if (animated)
		animation.deSerialize(is, protocol_ver);
}

void ParticleParameters::serialize(std::ostream &os, u16 protocol_ver) const
{
	writeV3F32(os, pos);
	writeV3F32(os, vel);
	writeV3F32(os, acc);
	writeF32(os, expirationtime);
	writeF32(os, size);
	writeU8(os, collisiondetection);
	os << serializeString32(texture.string);
	writeU8(os, vertical);
	writeU8(os, collision_removal);
	animation.serialize(os, 6); /* NOT the protocol ver */
	writeU8(os, glow);
	writeU8(os, object_collision);
	writeU16(os, node.param0);
	writeU8(os, node.param2);
	writeU8(os, node_tile);
	writeV3F32(os, drag);
}

void ParticleParameters::deSerialize(std::istream &is, u16 protocol_ver)
{
	pos                = readV3F32(is);
	vel                = readV3F32(is);
	acc                = readV3F32(is);
	expirationtime     = readF32(is);
	size               = readF32(is);
	collisiondetection = readU8(is);
	texture.string     = deSerializeString32(is);
	vertical           = readU8(is);
	collision_removal  = readU8(is);
	animation.deSerialize(is, 6); /* NOT the protocol ver */
	glow               = readU8(is);
	object_collision   = readU8(is);
	// This is kinda awful
	u16 tmp_param0 = readU16(is);
	if (is.eof())
		return;
	node.param0 = tmp_param0;
	node.param2 = readU8(is);
	node_tile   = readU8(is);
	if (protocol_ver >= 41) {
		drag = readV3F32(is);
	}
}

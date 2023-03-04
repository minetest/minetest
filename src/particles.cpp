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
#include <type_traits>
using namespace ParticleParamTypes;

#define PARAM_PVFN(n) ParticleParamTypes::n##ParameterValue
v2f PARAM_PVFN(pick) (float* f, const v2f a, const v2f b) {
	return v2f(
		numericalBlend(f[0], a.X, b.X),
		numericalBlend(f[1], a.Y, b.Y)
	);
}

v3f PARAM_PVFN(pick) (float* f, const v3f a, const v3f b) {
	return v3f(
		numericalBlend(f[0], a.X, b.X),
		numericalBlend(f[1], a.Y, b.Y),
		numericalBlend(f[2], a.Z, b.Z)
	);
}

v2f PARAM_PVFN(interpolate) (float fac, const v2f a, const v2f b)
	{ return b.getInterpolated(a, fac); }
v3f PARAM_PVFN(interpolate) (float fac, const v3f a, const v3f b)
	{ return b.getInterpolated(a, fac); }

#define PARAM_DEF_SRZR(T, wr, rd) \
	void PARAM_PVFN(serialize)  (std::ostream& os, T  v) {wr(os,v);  } \
	void PARAM_PVFN(deSerialize)(std::istream& is, T& v) {v = rd(is);}


#define PARAM_DEF_NUM(T, wr, rd) PARAM_DEF_SRZR(T, wr, rd) \
	T PARAM_PVFN(interpolate)(float fac, const T a, const T b) \
			{ return numericalBlend<T>(fac,a,b); } \
	T PARAM_PVFN(pick)       (float* f, const T a, const T b) \
			{ return numericalBlend<T>(f[0],a,b); }

PARAM_DEF_NUM(u8,  writeU8,    readU8);  PARAM_DEF_NUM(s8,  writeS8,    readS8);
PARAM_DEF_NUM(u16, writeU16,   readU16); PARAM_DEF_NUM(s16, writeS16,   readS16);
PARAM_DEF_NUM(u32, writeU32,   readU32); PARAM_DEF_NUM(s32, writeS32,   readS32);
PARAM_DEF_NUM(f32, writeF32,   readF32);
PARAM_DEF_SRZR(v2f, writeV2F32, readV2F32);
PARAM_DEF_SRZR(v3f, writeV3F32, readV3F32);

enum class ParticleTextureFlags : u8 {
	/* each value specifies a bit in a bitmask; if the maximum value
	 * goes above 1<<7 the type of the flags field must be changed
	 * from u8, which will necessitate a protocol change! */

	// the first bit indicates whether the texture is animated
	animated = 1,

	/* the next three bits indicate the blending mode of the texture
	 * blendmode is encoded by (flags |= (u8)blend << 1); retrieve with
	 * (flags & ParticleTextureFlags::blend) >> 1. note that the third
	 * bit is currently reserved for adding more blend modes in the future */
	blend = 0x7 << 1,
};

/* define some shorthand so we don't have to repeat ourselves or use
 * decltype everywhere */
using FlagT = std::underlying_type_t<ParticleTextureFlags>;

void ServerParticleTexture::serialize(std::ostream &os, u16 protocol_ver, bool newPropertiesOnly) const
{
	/* newPropertiesOnly is used to de/serialize parameters of the legacy texture
	 * field, which are encoded separately from the texspec string */
	FlagT flags = 0;
	if (animated)
		flags |= FlagT(ParticleTextureFlags::animated);
	if (blendmode != BlendMode::alpha)
		flags |= FlagT(blendmode) << 1;
	serializeParameterValue(os, flags);

	alpha.serialize(os);
	scale.serialize(os);
	if (!newPropertiesOnly)
		os << serializeString32(string);

	if (animated)
		animation.serialize(os, protocol_ver);
}

void ServerParticleTexture::deSerialize(std::istream &is, u16 protocol_ver, bool newPropertiesOnly)
{
	FlagT flags = 0;
	deSerializeParameterValue(is, flags);

	animated = !!(flags & FlagT(ParticleTextureFlags::animated));
	blendmode = BlendMode((flags & FlagT(ParticleTextureFlags::blend)) >> 1);

	alpha.deSerialize(is);
	scale.deSerialize(is);
	if (!newPropertiesOnly)
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
	animation.serialize(os, protocol_ver);
	writeU8(os, glow);
	writeU8(os, object_collision);
	writeU16(os, node.param0);
	writeU8(os, node.param2);
	writeU8(os, node_tile);
	writeV3F32(os, drag);
	jitter.serialize(os);
	bounce.serialize(os);
}

template <typename T, T (reader)(std::istream& is)>
inline bool streamEndsBeforeParam(T& val, std::istream& is)
{
	// This is kinda awful
	T tmp = reader(is);
	if (is.eof())
		return true;
	val = tmp;
	return false;
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
	animation.deSerialize(is, protocol_ver);
	glow               = readU8(is);
	object_collision   = readU8(is);

	if (streamEndsBeforeParam<u16, readU16>(node.param0, is))
		return;
	node.param2 = readU8(is);
	node_tile   = readU8(is);

	if (streamEndsBeforeParam<v3f, readV3F32>(drag, is))
		return;
	jitter.deSerialize(is);
	bounce.deSerialize(is);
}

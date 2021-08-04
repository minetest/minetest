/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <string>
#include <vector>
#include "irrlichttypes_bloated.h"
#include "tileanimation.h"
#include "mapnode.h"
#include "util/serialize.h"

// This file defines the particle-related structures that both the server and
// client need. The ParticleManager and rendering is in client/particles.h

namespace ParticleParamTypes {
	template<typename T> using BlendFunction = T(float,T,T);

	template <typename T,
			 void (S)(std::ostream&, T),
			 T    (D)(std::istream&)>
	struct SerializableParameter {
		using ValType = T;

		T val;
		using This = SerializableParameter<T,S,D>;

		SerializableParameter() = default;
		SerializableParameter(const This& a) : val(a.val) {};
		SerializableParameter(T b) { val = b; }

		operator T() const { return val; }
		T operator = (T b) { return val = b; }

		void serialize(std::ostream &os) const { S(os,val);   }
		void deSerialize(std::istream &is)     { val = D(is); }

		template <BlendFunction<T> Blend>
		static This blendWrap(float fac, This a, This b) {
			return This(Blend(fac, (T)a, (T)b));
		}
	};

	template <typename T>
	struct RangedParameter {
		using ValType = T;

		T min, max;

		RangedParameter() = default;
		RangedParameter(const RangedParameter<T>& a) : min(a.min), max(a.max) {};
		RangedParameter(T _min, T _max)              : min(_min),  max(_max)  {};
		template <typename M> RangedParameter(M b)   : min(b),     max(b)     {};

		void serialize(std::ostream &os) const { min.serialize(os);   max.serialize(os);   };
		void deSerialize(std::istream &is)     { min.deSerialize(is); max.deSerialize(is); };
	};

	template <typename T, BlendFunction<T> Blend>
	struct TweenedParameter {
		using ValType = T;

		T start, end;

		TweenedParameter() = default;
		TweenedParameter(const TweenedParameter<T,Blend>& a) : start(a.start), end(a.end) {};
		TweenedParameter(T _start, T _end)                   : start(_start),  end(_end) {};
		template <typename M> TweenedParameter(M b)          : start(b),       end(b) {};

		T blend(float fac) { return Blend(fac, start, end); }

		void serialize(std::ostream &os) const { start.serialize(os);   end.serialize(os);   };
		void deSerialize(std::istream &is)     { start.deSerialize(is); end.deSerialize(is); };
	};

	template <typename T> T numericalBlend(float fac, T min, T max) { return min + ((max - min) * fac); }

	v3f v3fBlend(float fac, v3f a, v3f b);

	template <typename T, BlendFunction<T> Blend>
	RangedParameter<T> rangeBlend(
			float fac,
			RangedParameter<T> a,
			RangedParameter<T> b
	) {
		RangedParameter<T> r;
		r.min = Blend(fac, a.min, b.min);
		r.max = Blend(fac, a.max, b.max);
		return r;
	}

	template <typename T, BlendFunction<T> Blend> using TweenedRangedParameter = TweenedParameter<
		/*      storage type */ RangedParameter<T>,
		/* blending function */ rangeBlend<T, Blend>
	>;


	// these aliases bind types to de/serialization functions
	using srz_v3f = SerializableParameter<v3f, writeV3F32, readV3F32>;
	using srz_f32 = SerializableParameter<f32, writeF32,   readF32>;

	// these are consistently-named convenience aliases to make code more readable without `using ParticleParamTypes` declarations
	using range_v3f = RangedParameter<srz_v3f>;
	using range_f32 = RangedParameter<srz_f32>;

	// these aliases bind tweened parameter types to the functions used to blend them
	using tween_f32       = TweenedParameter      <srz_f32, srz_f32::blendWrap<numericalBlend<f32> > >;
	// using tween_range_v3f = TweenedParameter<
	// 	RangedParameter<srz_v3f>,
	// 	rangeBlend<srz_v3f, srz_v3f::blendWrap<v3fBlend> >
	// >;
	using tween_range_v3f = TweenedRangedParameter<srz_v3f, srz_v3f::blendWrap<v3fBlend> >;
	using tween_range_f32 = TweenedRangedParameter<srz_f32, srz_f32::blendWrap<numericalBlend<f32> > >;
}

struct ParticleTexture {
	bool animated = false;
	TileAnimationParams animation;
	enum class Fade {
		none, in, out, pulse, flicker
	} fade_mode = Fade::none;
	u8 fade_freq = 1;
	f32 alpha = 1.0f, fade_start = 0.0f;
};

struct ServerParticleTexture : public ParticleTexture {
	std::string string;
};

struct CommonParticleParams {
	bool collisiondetection = false;
	bool collision_removal = false;
	bool object_collision = false;
	bool vertical = false;
	ServerParticleTexture texture;
	struct TileAnimationParams animation;
	u8 glow = 0;
	MapNode node;
	u8 node_tile = 0;

	CommonParticleParams() {
		animation.type = TAT_NONE;
		node.setContent(CONTENT_IGNORE);
	}

	/* This helper is useful for copying params from
	 * ParticleSpawnerParameters to ParticleParameters */
	inline void copyCommon(CommonParticleParams &to) const {
		to.collisiondetection = collisiondetection;
		to.collision_removal = collision_removal;
		to.object_collision = object_collision;
		to.vertical = vertical;
		to.texture = texture;
		to.animation = animation;
		to.glow = glow;
		to.node = node;
		to.node_tile = node_tile;
	}
};

struct ParticleParameters : CommonParticleParams {
	v3f pos;
	v3f vel;
	v3f acc;
	v3f drag;
	f32 expirationtime = 1;
	f32 size = 1;

	void serialize(std::ostream &os, u16 protocol_ver) const;
	void deSerialize(std::istream &is, u16 protocol_ver);
};

struct ParticleSpawnerParameters : CommonParticleParams {
	u16 amount = 1;
	f32 time = 1;

	// texture is used for the fallback texture on outdated clients

	std::vector<ServerParticleTexture> texpool;

	ParticleParamTypes::tween_range_v3f
		pos, vel, acc, drag, attractor;

	ParticleParamTypes::tween_range_f32
		exptime = 1, size = 1, attract = 1;

	// For historical reasons no (de-)serialization methods here
};

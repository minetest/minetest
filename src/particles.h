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
#include <ctgmath>
#include "irrlichttypes_bloated.h"
#include "tileanimation.h"
#include "mapnode.h"
#include "util/serialize.h"
#include "util/numeric.h"

// This file defines the particle-related structures that both the server and
// client need. The ParticleManager and rendering is in client/particles.h

namespace ParticleParamTypes {
	template<typename T> using BlendFunction = T(float,T,T);

// 	template <typename T,
// 			 void (S)(std::ostream&, T),
// 			 T    (D)(std::istream&)>
// 	struct SerializableParameter {
// 		using ValType = T;
//
// 		T val;
// 		using This = SerializableParameter<T,S,D>;
//
// 		SerializableParameter() = default;
// 		SerializableParameter(const This& a) : val(a.val) {};
// 		SerializableParameter(T b) { val = b; }
//
// 		operator T() const { return val; }
// 		T operator = (T b) { return val = b; }
//
// 		void serialize(std::ostream &os) const { S(os,val);   }
// 		void deSerialize(std::istream &is)     { val = D(is); }
//
// 		template <BlendFunction<T> Blend>
// 		static This blendWrap(float fac, This a, This b) {
// 			return This(Blend(fac, (T)a, (T)b));
// 		}
// 	};

	struct Parameter {
		virtual void serialize(std::ostream &os) const = 0;
		virtual void deSerialize(std::istream &is) = 0;
	};

	template <typename T, size_t PN>
	struct ParameterWrapper : public Parameter {
		using ValType = T;
		using pickFactors = float[PN];

		T val;
		using This = ParameterWrapper<T, PN>;

		ParameterWrapper() = default;
		ParameterWrapper(const This& a) : val(a.val) {};
		ParameterWrapper(T  b) : val(b) {};

		operator T() const { return val; }
		T operator = (T b) { return val = b; }
	};

	template <typename T> T numericalBlend(float fac, T min, T max)
		{ return min + ((max - min) * fac); }

	template <typename T>
	struct NumericParameter : public ParameterWrapper<T,1> {
		using This = NumericParameter<T>;

		template <typename... Args>
		NumericParameter(Args... args) : ParameterWrapper<T,1>(args...) {};

		void serialize(std::ostream &os) const { writeF32(os, this->val); }
		void deSerialize(std::istream &is)     { this->val = readF32(is); }
		This interpolate(float fac, const This against) const {
			return This(numericalBlend(fac, (T)*this, (T)against));
		}
		static This pick(float* f, const This a, const This b) {
			return a.interpolate(f[0], b);
		}
	};

	using f32Parameter = NumericParameter<f32>;
	struct v3fParameter : public ParameterWrapper<v3f,3> {
		template <typename... Args>
		v3fParameter(Args... args) : ParameterWrapper<v3f,3>(args...) {};

		void serialize(std::ostream &os) const { writeV3F32(os, this->val); }
		void deSerialize(std::istream &is)     { this->val = readV3F32(is); }
		v3fParameter interpolate(float fac, const v3fParameter against) const;
		static v3fParameter pick(float* f, const v3fParameter a, const v3fParameter b);
	};

	template <typename T>
	struct RangedParameter : public Parameter {
		using ValType = T;
		using This = RangedParameter<T>;

		T min, max;
		f32 bias;

		RangedParameter() = default;
		RangedParameter(const This& a)             : min(a.min), max(a.max) {};
		RangedParameter(T _min, T _max)            : min(_min),  max(_max)  {};
		template <typename M> RangedParameter(M b) : min(b),     max(b)     {};

		void serialize(std::ostream &os) const { min.serialize(os);   max.serialize(os);   };
		void deSerialize(std::istream &is)     { min.deSerialize(is); max.deSerialize(is); };

		This interpolate(float fac, const This against) const {
			This r;
			r.min = min.interpolate(fac, against.min);
			r.max = max.interpolate(fac, against.max);
			return r;
		}

		T pickWithin() {
			typename T::pickFactors values;
			auto p = numericAbsolute(bias);
			for (size_t i = 0; i < sizeof values / sizeof values[0]; ++i) {
				values[i] = pow(rand(), p);
			}
			return T::pick(values, min, max);
		}
	};

	template <typename T>
	struct TweenedParameter : public Parameter {
		using ValType = T;
		using This = TweenedParameter<T>;

		T start, end;

		TweenedParameter() = default;
		TweenedParameter(const This& a)             : start(a.start), end(a.end) {};
		TweenedParameter(T _start, T _end)          : start(_start),  end(_end) {};
		template <typename M> TweenedParameter(M b) : start(b),       end(b) {};

		T blend(float fac) { return start.interpolate(fac, end); }

		void serialize(std::ostream &os) const { start.serialize(os);   end.serialize(os);   };
		void deSerialize(std::istream &is)     { start.deSerialize(is); end.deSerialize(is); };
	};

// 	v3f v3fBlend(float fac, v3f a, v3f b);
//
// 	srz_v3f srz_v3f_pick(float* facs, srz_v3f min, srz_v3f max);

	// these aliases bind types to de/serialization functions
// 	using srz_v3f = SerializableParameter<v3f, writeV3F32, readV3F32>;
// 	using srz_f32 = SerializableParameter<f32, writeF32,   readF32>;

	// these are consistently-named convenience aliases to make code more readable without `using ParticleParamTypes` declarations
	using v3fRange = RangedParameter<v3fParameter>;
	using f32Range = RangedParameter<f32Parameter>;

	// these aliases bind tweened parameter types to the functions used to blend them
	using v3fTween      = TweenedParameter<v3fParameter>;
	using f32Tween      = TweenedParameter<f32Parameter>;
	using v3fRangeTween = TweenedParameter<v3fRange>;
	using f32RangeTween = TweenedParameter<f32Range>;
}

struct ParticleTexture {
	bool animated = false;
	TileAnimationParams animation;
	enum class Fade {
		none, in, out, pulse, flicker
	} fade_mode = Fade::none;
	u8 fade_reps = 1;
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

	ParticleParamTypes :: v3fRangeTween
		pos, vel, acc, drag, radius;

	ParticleParamTypes :: v3fTween
		attractor;

	ParticleParamTypes :: f32RangeTween
		exptime = (f32)1, size = (f32)1, attract = (f32)0;

	// For historical reasons no (de-)serialization methods here
};

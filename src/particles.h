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
#include <sstream>
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
	#define DECL_PARAM_SRZRS(type) \
		void serializeParameterValue  (std::ostream& os, type   v); \
		void deSerializeParameterValue(std::istream& is, type&  r);
	#define DECL_PARAM_OVERLOADS(type) DECL_PARAM_SRZRS(type) \
		type interpolateParameterValue(float  fac,  const type a, const type b); \
		type pickParameterValue       (float* facs, const type a, const type b);

	DECL_PARAM_OVERLOADS(u8);
	DECL_PARAM_OVERLOADS(u16);
	DECL_PARAM_OVERLOADS(u32);
	DECL_PARAM_OVERLOADS(f32);
	DECL_PARAM_OVERLOADS(v2f);
	DECL_PARAM_OVERLOADS(v3f);

	template <typename T, size_t PN>
	struct Parameter {
		using ValType = T;
		using pickFactors = float[PN];

		T val;
		using This = Parameter<T, PN>;

		Parameter() = default;
		Parameter(const This& a) : val(a.val) {};
		template <typename... Args>
		Parameter(Args... args) : val(args...) {};

		virtual void serialize(std::ostream &os) const
			{ serializeParameterValue  (os, this->val); }
		virtual void deSerialize(std::istream &is)
			{ deSerializeParameterValue(is, this->val); }

		virtual T interpolate(float fac, const This& against) const {
			return interpolateParameterValue(fac, this->val, against.val);
		}

		static T
		pick(float* f, const This& a, const This& b) {
			return pickParameterValue(f, a.val, b.val);
		}

		operator T() const { return val; }
		T operator = (T b) { return val = b; }

	};

	template <typename T> T numericalBlend(float fac, T min, T max)
		{ return min + ((max - min) * fac); }


	template <typename T, size_t N>
	struct VectorParameter : public Parameter<T,N> {
		using This = VectorParameter<T,N>;
		template <typename... Args>
		VectorParameter(Args... args) : Parameter<T,N>(args...) {};
	};

	template <typename T, size_t PN>
	std::string dump(const Parameter<T,PN>& p) {
		std::stringstream s;
		s << p.val;
		return s.str();
	}

	template <typename T, size_t N>
	std::string dump(const VectorParameter<T,N>& v) {
		std::stringstream s;
		s << "vec"<<N<<"<"<< v.val.X
					<< "," << v.val.Y;
		if (N==3) {
			s << "," << v.val.Z;
		}
		s << ">";
		return s.str();
	}

	using u8Parameter  = Parameter<u8,  1>;
	using u16Parameter = Parameter<u16, 1>;
	using u32Parameter = Parameter<u32, 1>;

	using f32Parameter = Parameter<f32, 1>;

	using v2fParameter = VectorParameter<v2f, 2>;
	using v3fParameter = VectorParameter<v3f, 3>;

	template <typename T>
	struct RangedParameter {
		using ValType = T;
		using This = RangedParameter<T>;

		T min, max;
		f32 bias = 0;

		RangedParameter() = default;
		RangedParameter(const This& a)             : min(a.min), max(a.max) {};
		RangedParameter(T _min, T _max)            : min(_min),  max(_max)  {};
		template <typename M> RangedParameter(M b) : min(b),     max(b)     {};

		// these functions handle the old range serialization "format"; bias must
		// be manually encoded in a separate part of the stream. NEVER ADD FIELDS
		// TO THESE FUNCTIONS
		void legacySerialize(std::ostream& os) const {
			min.serialize(os);
			max.serialize(os);
		}
		void legacyDeSerialize(std::istream& is) {
			min.deSerialize(is);
			max.deSerialize(is);
		}

		// these functions handle the format used by new fields. new fields go here
		void serialize(std::ostream &os) const {
			legacySerialize(os);
			writeF32(os, bias);
		};
		void deSerialize(std::istream &is) {
			legacyDeSerialize(is);
			bias = readF32(is);
		};

		This interpolate(float fac, const This against) const {
			This r;
			r.min = min.interpolate(fac, against.min);
			r.max = max.interpolate(fac, against.max);
			return r;
		}

		T pickWithin() {
			typename T::pickFactors values;
			auto p = numericAbsolute(bias) + 1;
			for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); ++i) {
				if (bias < 0)
					values[i] = 1.0f - pow(myrand_float(), p);
				else
					values[i] = pow(myrand_float(), p);
			}
			return T::pick(values, min, max);
		}

	};

	template <typename T>
	std::string dump(const RangedParameter<T>& r) {
		std::stringstream s;
		s << "range<" << dump(r.min) << " ~ " << dump(r.max);
		if (r.bias != 0)
			s << " :: " << r.bias;
		s << ">";
		return s.str();
	}

	enum class TweenStyle { fwd, rev, pulse, flicker };

	template <typename T>
	struct TweenedParameter {
		using ValType = T;
		using This = TweenedParameter<T>;

		TweenStyle style = TweenStyle::fwd;
		u16 reps = 1;
		f32 beginning = 0.0f;

		T start, end;

		TweenedParameter() = default;
		TweenedParameter(const This& a) = default;
		TweenedParameter(T _start, T _end)          : start(_start),  end(_end) {};
		template <typename M> TweenedParameter(M b) : start(b),       end(b) {};

		T blend(float fac) const {
			// warp time coordinates in accordance w/ settings
			if (fac > beginning) {
				// remap for beginning offset
				auto len = 1 - beginning;
				fac -= beginning;
				fac /= len;

				// remap for repetitions
				fac *= reps;
				if (fac > 1) // poor man's modulo
					fac -= (decltype(reps))fac;

				// remap for style
				switch (style) {
					case TweenStyle::fwd: /* do nothing */  break;
					case TweenStyle::rev: fac = 1.0f - fac; break;
					case TweenStyle::pulse:
					case TweenStyle::flicker: {
						if (fac > 0.5f) {
							fac = 1.f - (fac*2.f - 1.f);
						} else {
							fac = fac * 2;
						}
						if (style == TweenStyle::flicker) {
							fac *= myrand_range(0.7f, 1.0f);
						}
					}
				}
				if (fac>1.f)
					fac = 1.f;
				else if (fac<0.f)
					fac = 0.f;
			} else {
				fac = (style == TweenStyle::rev) ? 1.f : 0.f;
			}

			return start.interpolate(fac, end);
		}

		void serialize(std::ostream &os) const {
			writeU8(os, (u8)style);
			writeU16(os, reps);
			writeF32(os, beginning);
			start.serialize(os);
			end.serialize(os);
		};
		void deSerialize(std::istream &is) {
			u8 st = readU8(is);
			style = (TweenStyle)st;
			reps = readU16(is);
			beginning = readF32(is);
			start.deSerialize(is);
			end.deSerialize(is);
		};

	};

	template <typename T>
	std::string dump(const TweenedParameter<T>& t) {
		std::stringstream s;
		const char* icon;
		switch (t.style) {
			case TweenStyle::fwd: icon = "→"; break;
			case TweenStyle::rev: icon = "←"; break;
			case TweenStyle::pulse: icon = "↔"; break;
			case TweenStyle::flicker: icon = "↯"; break;
		}
		s << "tween<";
		if (t.reps != 1)
			s << t.reps << "x ";
		s << dump(t.start) << " "<<icon<<" " << dump(t.end) << ">";
		return s.str();
	}

	enum class AttractorKind { none, point, line, plane };
	DECL_PARAM_SRZRS(AttractorKind);

	// these are consistently-named convenience aliases to make code more readable without `using ParticleParamTypes` declarations
	using v3fRange = RangedParameter<v3fParameter>;
	using f32Range = RangedParameter<f32Parameter>;

	// these aliases bind tweened parameter types to the functions used to blend them
	using v2fTween      = TweenedParameter<v2fParameter>;
	using v3fTween      = TweenedParameter<v3fParameter>;
	using f32Tween      = TweenedParameter<f32Parameter>;
	using v3fRangeTween = TweenedParameter<v3fRange>;
	using f32RangeTween = TweenedParameter<f32Range>;

	#undef DECL_PARAM_SRZRS
	#undef DECL_PARAM_OVERLOADS
}

struct ParticleTexture {
	bool animated = false;
	TileAnimationParams animation;
	ParticleParamTypes::f32Tween alpha = (f32)1;
	ParticleParamTypes::v2fTween scale; //= (v2f){1.f,1.f};

	ParticleTexture();
};

struct ServerParticleTexture : public ParticleTexture {
	std::string string;
	void serialize(std::ostream &os, u16 protocol_ver, bool newPropertiesOnly = false) const;
	void deSerialize(std::istream &is, u16 protocol_ver, bool newPropertiesOnly = false);
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
	ParticleParamTypes::f32Range bounce;

	void serialize(std::ostream &os, u16 protocol_ver) const;
	void deSerialize(std::istream &is, u16 protocol_ver);
};

struct ParticleSpawnerParameters : CommonParticleParams {
	u16 amount = 1;
	f32 time = 1;

	std::vector<ServerParticleTexture> texpool;

	ParticleParamTypes::v3fRangeTween
		pos, vel, acc, drag, radius;

	ParticleParamTypes::AttractorKind
		attractor_kind;
	ParticleParamTypes::v3fTween
		attractor, attractor_angle;
	u16 attractor_attachment = 0, /* object IDs */
	    attractor_angle_attachment = 0;
	bool attractor_kill = true;
	/* do particles disappear when they cross the attractor threshold? */

	ParticleParamTypes::f32RangeTween
		exptime = (f32)1,
		size    = (f32)1,
		attract = (f32)0,
		bounce  = (f32)0;

	// For historical reasons no (de-)serialization methods here
};

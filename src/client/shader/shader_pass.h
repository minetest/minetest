/*
Minetest
Copyright (C) 2010-2021

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

#include "client/shader/shader_common.h"
#include "client/shader/shader_program.h"

struct ShaderFeatureRing {
	u32 index;	// Number to left-shift
	u32	shift;	// Bit count to left-shift
	u64 mask;	// Mask of the place where ring lives
	inline void DisableFeature( u64 *key ) {
		*key &= ~mask;
	}
	inline void EnableFeature( u64 *key ) {
		*key = ( *key & ~mask ) | index << shift;
	}
}

/*
	A single utility pass of a shader.
	Utility passes allow a shader to provide
	different shader programs for a given rendering technique.

	Examples of utility passes:
	- G-buffer setup
	- Shadow caster
	- Plain forward render
	- Outline or other highlight
	- Damage overlay or similar
*/
class ShaderPass {
	ShaderSource sources;

	// Variant keys are 64 bit unsigned values.
	// A variant key maps to a single pass variant.

	// The key is a packed bitfield of several numbers,
	// which span variable bit widths.

	// A single toggle feature occupies a single bit,
	// and a ring of mutually exclusive features occupies
	// the minimum amount of bits needed to store it.
	// A binary feature is thus a ring with a bit width of 1.

	// Mutex rings always generate a zero variant where none of the
	// defines are enabled. Mind that when writing shaders.
	// This variant may be selected by disabling any of the matching keywords.

	// Rings are stored implicitly, in the form of
	// bitmasks and bitshift offsets.
	std::unordered_map<std::string,ShaderFeatureRing> featureMap;
	std::vector<ShaderProgram*> variants;

	bool buildFailed;

	void DeleteVariants();

	// Generate all variants recursively from a list of features.
	void GenVariants( const ShaderSource &sources, const std::vector<std::string> &features );
public:

	inline void DisableFeature( const std::string &name, u64 *key ) {
		featureRings.at( name ).DisableFeature( key );
	}
	inline void EnableFeature( const std::string &name, u64 *key {
		featureRings.at( name ).EnableFeature( key );
	}

	FixedFunctionState fixedState;

	inline u32 GetVariantCount() const {
		return variants.size();
	}

	inline bool IsValid() const {
		return !buildFailed;
	}

	// Build a bitmask based on a list of features.
	u64 GetVariantKey( const std::vector<std::string> featuresUsed ) const;
	// Get a program based on a variant bitmask obtained from GetVariantKey.
	const ShaderProgram& GetProgram( const u64 programKey ) const;

	ShaderPass( const ShaderSource &sources, const std::vector<std::string> &features );

	inline ~ShaderPass() {
		DeleteVariants();
	}
};

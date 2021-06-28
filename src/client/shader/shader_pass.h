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

/*
	Sources for a single pass.
*/
struct PassSources {
	std::string header; // Common header pasted right above each shader stage.

	std::string stages[5];
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
	PassSources sources;

	// Each shader feature is assigned a bit position for use in a bitmask.
	// Then, each unique bitmask is a key to a map of variant programs.
	std::unordered_map<std::string,int> featurePositions;
	std::vector<ShaderProgram> variants;

	u32 variantCount;

	std::stringstream errorLog;

	bool buildFailed;

	// Generate all variants recursively from a list of features.
	void GenVariants( std::vector<std::string> &featureStack );
public:

	FixedFunctionState fixedState;

	inline u32 GetVariantCount() { return variantCount; }

	inline bool IsValid() { return !buildFailed; }

	// Build a bitmask based on a list of features.
	const u64 GetVariantKey( std::vector<std::string> featuresUsed ) const;
	// Get a program based on a variant bitmask obtained from GetVariantKey.
	const ShaderProgram& GetProgram( const u64 programKey ) const;

	ShaderPass( const PassSources sources, const std::vector<std::string> features );
};

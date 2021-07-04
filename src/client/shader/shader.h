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
#include "client/shader/shader_pass.h"

extern const std::unordered_map< u32, u32 > uniformTypeStrides;

class Shader {
	// Eliminates the need for a lot of ugly accessors.
	friend class Material;

	u32 passCount;
	std::unordered_map<std::string, u32> passMap;
	std::vector<ShaderPass> passes;

	u32 uniformCount;
	// Uniform indices by name, just for convenience.
	std::unordered_map<std::string, u32> uniformIndexMap;
	// Uniform names in order.
	std::vector<std::string> uniformNames;
	// Type of each uniform, as understood by GL
	std::vector<u32> uniformTypes;
	// Amount of each primitive in a uniform. This is array lengths * vector widths;
	std::vector<u32> uniformWidths;
	// Size in bytes of a contiguous buffer that can store all the uniform state
	// required by this shader. It is a sum of all strides.
	size_t uniformBufferSize;
	// Offset of each uniform in the aforementioned buffer.
	std::vector<uintptr_t> uniformMemoryOffsets;

	/*
		The Location Matrix

		This is a 3-dimensional jagged array of uniform locations.
		The first two indices are pass index and variant key, and
		the third index is the uniform ID as understood by this class.

		To utilize this, you retrieve the appropriate row using the
		pass and variant, and then bring up this row to the material's
		list of uniform values. Then you iterate one by one, setting uniforms
		to the locations retrieved from the row, types known by the shader,
		and values known by the material, skipping wherever the location
		equals -1 (which means this particular Program did not contain
		this specific uniform after linking).
	*/
	std::vector<std::vector<std::vector<s32> > > locationMatrix;

	// Rebuild all uniform data.
	void BuildUniformData();

	// For force-enabling features globally
	std::vector<u64> enableMask;
	// For force-disabling features globally
	std::vector<u64> disableMask;

public:
	inline s32 GetUniformIndex( const std::string &name ) const {
		return STL_AT_OR( uniformIndexMap, name, -1 );
	}
	inline const ShaderProgram &GetProgram( u32 passId, u64 variant, u64 &outActualVariant ) const {
		outActualVariant = (variant & disableMask[passId]) | enableMask[passId];
		return passes[passId].GetProgram( outActualVariant );
	}

	inline void SetForceEnableFeature( const std::string &name ) {
		for ( u32 i=0; i < passCount; i++ )
			passes[i].EnableFeature( name, enableMask.data + i );
	}
	inline void ClearForceEnableFeature( const std::string &name ) {
		for ( u32 i=0; i < passCount; i++ )
			passes[i].DisableFeature( name, enableMask.data + i );
	}
	inline void SetForceDisableFeature( const std::string &name ) {
		for ( u32 i=0; i < passCount; i++ )
			disableMask[i] &= ~passes[i].GetFeatureRingMask( name );
	}
	inline void ClearForceDisableFeature( const std::string &name ) {
		for ( u32 i=0; i < passCount; i++ )
			disableMask[i] |= passes[i].GetFeatureRingMask( name );
	}

	Shader( const std::unordered_map<std::string,ShaderSource> &sources );
};

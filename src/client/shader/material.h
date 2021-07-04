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

#include "client/shader/shader.h"
#include <matrix4.h>

class Material {
	const Shader *shader;

	// Contiguous buffer of memory for all the uniforms.
	u8 *uniformMemory;

	// Blit a uniform into the given index.
	// This function assumes that all the necessary safety checks
	// have already been performed, and 'value' is a pointer to an
	// appropriate value for the uniform with the index 'i'.
	void SetUniformUnsafe( const s32 i, const void *value );

	// Variant state for each pass.
	std::vector<u64> currentVariants;

	// Fixed state for each pass.
	// Fixed function state is not preserved between shader reassigns!
	// (this may be changed if requested, but it's a lot of API)
	// For now, this is initialized with the values in the respective passes,
	std::vector<FixedFunctionState> fixedStates;

	u32 uniformCount;

public:
	std::unordered_map<std::string, bool> features;

	// Get the index for the uniform 'name', or -1 if not present.
	// Please keep in mind that this index is only valid for a
	// specific shader.
	inline s32 GetUniformIndex( const std::string &name ) {
		return shader->GetUniformIndex( name );
	}

	// Uniform setters.
	void SetFloat( s32 index, float v );
	void SetFloat2( s32 index, const float *v );
	void SetFloat2( s32 index, const v2f &v );
	void SetFloat2( s32 index, const v2s16 &v );
	void SetFloat3( s32 index, const float *v );
	void SetFloat3( s32 index, const v3f &v );
	void SetFloat3( s32 index, const v3s16 &v );
	void SetFloat4( s32 index, const float *v );
	void SetMatrix( s32 index, const float *v );
	void SetMatrix( s32 index, const irr::core::matrix4 &v );
	void SetTexture( s32 index, u32 texture );
	inline void SetFloat( const std::string &name, const float v ){
		SetFloat( GetUniformIndex( name ), v );
	}
	inline void SetFloat2( const std::string &name, const float *v ){
		SetFloat2( GetUniformIndex( name ), v );
	}
	inline void SetFloat2( const std::string &name, const v2f &v ){
		SetFloat2( GetUniformIndex( name ), v );
	}
	inline void SetFloat2( const std::string &name, const v2s16 &v ){
		SetFloat2( GetUniformIndex( name ), v );
	}
	inline void SetFloat3( const std::string &name, const float *v ){
		SetFloat3( GetUniformIndex( name ), v );
	}
	inline void SetFloat3( const std::string &name, const v3f &v ){
		SetFloat3( GetUniformIndex( name ), v );
	}
	inline void SetFloat3( const std::string &name, const v3s16 &v ){
		SetFloat3( GetUniformIndex( name ), v );
	}
	inline void SetFloat4( const std::string &name, const float *v ){
		SetFloat4( GetUniformIndex( name ), v );
	}
	inline void SetMatrix( const std::string &name, const float *v ){
		SetMatrix( GetUniformIndex( name ), v );
	}
	inline void SetMatrix( const std::string &name, const irr::core::matrix4 &v ){
		SetMatrix( GetUniformIndex( name ), v );
	}
	inline void SetTexture( const std::string &name, const u32 texture ){
		SetTexture( GetUniformIndex( name ), texture );
	}

	// Give the Material a new shader.
	void SetShader( const Shader *newShader );

	// Get the pass index or -1 if not
	inline s32 GetPassID( const std::string &passName ) {
		return STL_AT_OR( shader->passMap, passName, -1 );
	}

	// Retrieve the program, bind it, set all necessary uniforms,
	// and any other state
	void BindForRendering( u32 passId );
	inline void BindForRendering( const std::string &passName ) {
		if ( STL_CONTAINS( shader->passMap, passName ) )
			BindForRendering( shader->passMap.at( passName ) );
	}

	inline void EnableFeature( std::string feature ) {
		for ( u32 i = 0; i < shader->passCount; i++ ) {
			shader->passes[i].EnableFeature( feature, currentVariants.data() + i )
		}
	}
	inline void DisableFeature( std::string feature ) {
		for ( u32 i = 0; i < shader->passCount; i++ ) {
			shader->passes[i].DisableFeature( feature, currentVariants.data() + i )
		}
	}

	Material( const Shader *shader ) { SetShader( shader ); }
};

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

class Material {
	Shader *shader;

	// Contiguous buffer of memory for all the uniforms.
	u8 *uniformMemory;

	// Blit a uniform into the given index.
	// This function assumes that all the necessary safety checks
	// have already been performed, and 'value' is a pointer to an
	// appropriate value for the uniform with the index 'i'.
	void SetUniformUnsafe( const uint i, const void *value );

	// Variant state for each pass.
	std::vector<u64> currentVariants;

	u32 uniformCount;

public:
	std::unordered_map<std::string, std::bool> features;

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
	void SetMatrix( s32 index, const Matrix4f &v );
	void SetTexture( s32 index, u32 texture );
	inline void SetFloat( const std::string &name, float v ){
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
	inline void SetMatrix( const std::string &name, const Matrix4f &v ){
		SetMatrix( GetUniformIndex( name ), v );
	}
	inline void SetTexture( const std::string &name, u32 texture ){
		SetTexture( GetUniformIndex( name ), v );
	}

	// Give the Material a new shader.
	void SetShader( const Shader &shader );

	// Retrieve the program, bind it and set the relevant uniforms.
	void BindForRendering( u32 passId );

	Material( const Shader &shader ) { SetShader( shader ); }
};

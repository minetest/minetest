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

#include "client/shader/material.h"

void Material::SetFloat( s32 index, float v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat2( s32 index, float *const v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_VEC2 )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat2( s32 index, v2f v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_VEC2 )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat2( s32 index, v2s16 v ){
	v2f tmp = v;
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_VEC2 )
		SetUniformUnsafe( index, &tmp );
}
void Material::SetFloat3( s32 index, const float *v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_VEC3 )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat3( s32 index, v3f v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_VEC3 )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat3( s32 index, v3s16 v ){
	v3f tmp = v;
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_VEC3 )
		SetUniformUnsafe( index, &tmp );
}
void Material::SetFloat4( s32 index, const float *v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_VEC4 )
		SetUniformUnsafe( index, &v );
}
void Material::SetMatrix( s32 index, const float *v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_MAT4 )
		SetUniformUnsafe( index, &v );
}
void Material::SetMatrix( s32 index, const Matrix4f &v ){
	if ( index > -1 && index < uniformCount && uniformTypes[index] == GL_FLOAT_MAT4 )
		SetUniformUnsafe( index, v.pointer() );
}
void Material::SetTexture( s32 index, u32 texture ){
	if ( index > -1 && index < uniformCount && (
		uniformTypes[index] == GL_SAMPLER_2D ||
		uniformTypes[index] == GL_SAMPLER_3D ||
		uniformTypes[index] == GL_SAMPLER_CUBE ) )
	{
		SetUniformUnsafe( index, v.pointer() );
	}
}
void Material::SetUniformUnsafe( const uint i, const void *value ) {
	uintptr_t destAddress = ((uintptr_t)uniformMemory) + uniformMemoryOffsets[i];
	std::memcpy( (void*)destAddress, value, shader->uniformTypeStrides[shader->uniformTypes[i]] );
}
void Material::BindForRendering( u32 passId ) {
	auto &program = shader->GetProgram( passId, currentVariants[passId] );
}
void Material::SetShader( const Shader *newShader ) {
	u8 *newUniformMemory = new u8[newShader->GetUniformBufferSize()];
	if ( uniformMemory ) {
		// Copy what we can from the old shader
		for ( auto &pair : newShader->uniformIndexMap ) {
			// Does the old shader have a uniform with this name?
			if ( STL_CONTAINS( shader->uniformIndexMap, pair.first ) ) {
				auto &oldIndex = shader->uniformIndexMap.at( pair.first );
				// Is it the same type?
				auto oldT = shader->uniformTypes[oldIndex];
				auto newT = newShader->uniformTypes[pair.second];
				if ( oldT == newT ) {
					// Blit the value
					std::memcpy(
						newUniformMemory + newShader->uniformMemoryOffsets[pair.second],
						uniformMemory + shader->uniformMemoryOffsets[oldIndex],
						uniformTypeStrides[oldT]
					);
				}
			}
		}
		delete uniformMemory;
	}

	uniformMemory = newUniformMemory;
	this->shader = newShader;

	uniformCount = shader->GetUniformCount();

	// todo: Preserve variants the same way uniforms are
	currentVariants.clear();
	for( int i = 0; i < shader->variantCount; i++ ) {
		currentVariants.push_back( 0ULL );
	}
}

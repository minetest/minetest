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

void Material::SetFloat( s32 index, const float v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat2( s32 index, const float *v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_VEC2 )
		SetUniformUnsafe( index, v );
}
void Material::SetFloat2( s32 index, const v2f &v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_VEC2 )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat2( s32 index, const v2s16 &v ){
	v2f tmp = { (float)v.X, (float)v.Y };
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_VEC2 )
		SetUniformUnsafe( index, &tmp );
}
void Material::SetFloat3( s32 index, const float *v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_VEC3 )
		SetUniformUnsafe( index, v );
}
void Material::SetFloat3( s32 index, const v3f &v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_VEC3 )
		SetUniformUnsafe( index, &v );
}
void Material::SetFloat3( s32 index, const v3s16 &v ){
	v3f tmp = { (float)v.X, (float)v.Y, (float)v.Z };
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_VEC3 )
		SetUniformUnsafe( index, &tmp );
}
void Material::SetFloat4( s32 index, const float *v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_VEC4 )
		SetUniformUnsafe( index, v );
}
void Material::SetMatrix( s32 index, const float *v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_MAT4 )
		SetUniformUnsafe( index, v );
}
void Material::SetMatrix( s32 index, const irr::core::matrix4 &v ){
	if ( index > -1 && index < (s32)uniformCount && shader->uniformTypes[index] == GL_FLOAT_MAT4 )
		SetUniformUnsafe( index, v.pointer() );
}
void Material::SetTexture( s32 index, const u32 texture ){
	if ( index > -1 && index < (s32)uniformCount && (
		shader->uniformTypes[index] == GL_SAMPLER_2D ||
		shader->uniformTypes[index] == GL_SAMPLER_3D ||
		shader->uniformTypes[index] == GL_SAMPLER_CUBE ) )
	{
		SetUniformUnsafe( index, &texture );
	}
}
void Material::SetUniformUnsafe( const s32 i, const void *value ) {
	uintptr_t destAddress = ((uintptr_t)uniformMemory) + shader->uniformMemoryOffsets[i];
	std::memcpy( (void*)destAddress, value, uniformTypeStrides.at(shader->uniformTypes[i]) );
}
void Material::BindForRendering( u32 passId ) {
	if ( passId < 0 || passId > shader->passCount ) {
			warningstream << "Material::BindForRendering was asked to use a nonexistent pass\n";
			return;
	}
	u64 actualVariantId = 0;
	auto &program = shader->GetProgram( passId, currentVariants[passId], actualVariantId );
	program.Bind();
	u32 gpuTextureUnit = 0;
	auto &locations = shader->locationMatrix[passId][actualVariantId];
	// Iterate over *all* the uniforms known to this shader, in all passes
	// see locationMatrix declaration in shader.h
	for ( u32 i = 0; i < shader->uniformCount; i++ ) {
		s32 loc = locations[i];
		// Does this particular program have this uniform? (otherwise location is -1)
		if ( 0 <= loc ) {
			void *uPtr = reinterpret_cast<void*>( uniformMemory + shader->uniformMemoryOffsets[i] );
			u32 type = shader->uniformTypes[i];
			s32 len = shader->uniformWidths[i];
			switch( type ) {
				case GL_FLOAT:
					glUniform1fv( loc, len, reinterpret_cast<float*>(uPtr) );
					break;
				case GL_INT:
					glUniform1iv( loc, len, reinterpret_cast<s32*>(uPtr) );
					break;
				case GL_TEXTURE_CUBE_MAP:
				case GL_TEXTURE_3D:
				case GL_TEXTURE_2D:
					// Without ARB_bindless_texture we have to select
					// a hardware texture unit, bind the actual texture,
					// and then point the uniform to said hardware unit.
					// Dumb, I know.
					glActiveTexture( GL_TEXTURE0 + gpuTextureUnit );
					glBindTexture( type, *(reinterpret_cast<s32*>(uPtr)) );
					glUniform1i( loc, gpuTextureUnit++ );
					break;
				default:
			}
		}
	}
	// Set fixed function state.
	fixedStates[passId].Set();
}
void Material::SetShader( const Shader *newShader ) {
	u8 *newUniformMemory = new u8[newShader->uniformBufferSize()];
	if ( uniformMemory ) {
		// Copy what we can from the old shader
		for ( auto &pair : newShader->uniformIndexMap ) {
			auto &name = pair.first;
			auto &index = pair.second;
			// Does the old shader have a uniform with this name?
			if ( STL_CONTAINS( shader->uniformIndexMap, name ) ) {
				auto &oldIndex = shader->uniformIndexMap.at( name );
				// Is it the same type?
				auto oldT = shader->uniformTypes[oldIndex];
				auto newT = newShader->uniformTypes[index];
				if ( oldT == newT ) {
					// Blit the value
					std::memcpy(
						newUniformMemory + newShader->uniformMemoryOffsets[index],
						uniformMemory + shader->uniformMemoryOffsets[oldIndex],
						uniformTypeStrides.at(oldT)
					);
				}
			}
		}
		delete uniformMemory;
	}

	uniformMemory = newUniformMemory;
	this->shader = newShader;

	uniformCount = shader->uniformCount;

	// todo: Preserve variants the same way uniforms are
	currentVariants.clear();
	for( u32 i = 0; i < shader->passes.size(); i++ ) {
		currentVariants.push_back( 0ULL );
	}
}

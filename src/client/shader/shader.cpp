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

#include "client/shader/shader.h"
#include "log.h"

const std::unordered_map< u32, u32 > uniformTypeStrides = {
	{ GL_FLOAT, 		sizeof( GLfloat ) },
	// { GL_FLOAT_VEC2,	sizeof( GLfloat ) * 2 },
	// { GL_FLOAT_VEC3,	sizeof( GLfloat ) * 3 },
	// { GL_FLOAT_VEC4,	sizeof( GLfloat ) * 4 },
	// { GL_FLOAT_MAT2,	sizeof( GLfloat ) * 4 },
	// { GL_FLOAT_MAT3,	sizeof( GLfloat ) * 9 },
	// { GL_FLOAT_MAT4,	sizeof( GLfloat ) * 16 },
	{ GL_INT, 			sizeof( GLint ) },
	// { GL_INT_VEC2,		sizeof( GLint ) * 2 },
	// { GL_INT_VEC3,		sizeof( GLint ) * 3 },
	// { GL_INT_VEC4,		sizeof( GLint ) * 4 },
	{ GL_BOOL, 			sizeof( GLint ) },
	// { GL_BOOL_VEC2,		sizeof( GL ) * 2 },
	// { GL_BOOL_VEC3,		sizeof( GL ) * 3 },
	// { GL_BOOL_VEC4,		sizeof( GL ) * 4 },

	// Note: enums below are intentionally "wrong";
	// see ShaderProgram::GetUniforms( )
	{ GL_TEXTURE_2D,		sizeof( GLint ) },
	{ GL_TEXTURE_3D,		sizeof( GLint ) },
	{ GL_TEXTURE_CUBE_MAP,	sizeof( GLint ) },
};

void Shader::BuildUniformData() {

	std::unordered_set<std::string> alreadyWarned;
	std::unordered_map<std::string,u32> allUniformTypes;
	std::unordered_map<std::string,u32> allArrayLengths;

	// Grand uniform matrix in a yet-unresolved form.
	std::vector<std::vector<std::unordered_map<std::string,ProgramUniform> > > allProgramUniforms;

	for ( auto &pass : passes ) {
		std::vector<std::unordered_map<std::string,ProgramUniform> > passUniforms;
		for ( u32 i = 0; i < pass.GetVariantCount(); i++ ) {
			std::unordered_map<std::string,ProgramUniform> programUniforms;
			auto &program = pass.GetProgram( i );
			programUniforms = program.GetUniforms();
			for ( auto &pair : programUniforms ) {
				if ( !STL_CONTAINS( allUniformTypes, pair.first ) ) {
					allUniformTypes[pair.first] = pair.second.type;
					allArrayLengths[pair.first] = pair.second.arrayLength;
				} else {
					if ( allUniformTypes[pair.first] != pair.second.type
						&& !STL_CONTAINS( alreadyWarned, pair.first ) ) {
						warningstream << "Type of uniform " << pair.first
							<< " is not the same between passes or variants."
							<< " This is unsupported and may result"
							<< " in undefined behavior.\n";
						alreadyWarned.insert(pair.first);
					}
				}
			}
			passUniforms.push_back(programUniforms);
		}
		allProgramUniforms.push_back( passUniforms );
	}

	uniformCount = 0;
	for ( auto &pair : allUniformTypes ) {
		// This is where the Shader's uniform data crystallizes.
		// Indices are fixed from now on.
		uniformTypes.push_back( pair.second );
		uniformNames.push_back( pair.first );
		uniformWidths.push_back( allArrayLengths.at(pair.first) );
		uniformIndexMap[pair.first] = uniformCount;
		uniformCount++;
	}

	// Iterate over all uniforms again.
	for ( u32 i = 0; i < passes.size(); i++ ) {
		auto &pass = passes[i];
		std::vector<std::vector<s32> > locationColumn;
		for ( u32 j = 0; j < pass.GetVariantCount(); j++ ) {
			std::vector<s32> locationRow;
			auto &programUniforms = allProgramUniforms[i][j];
			for ( u32 k = 0; k < uniformNames.size(); k++ ) {
				s32 uniformLocation;
				if ( STL_CONTAINS( programUniforms, uniformNames[k] ) ) {
					// The uniform with this name has the following location in this program:
					uniformLocation = programUniforms.at( uniformNames[k] ).location;
				} else {
					// A uniform with this name is not present in this particular program.
					uniformLocation = -1;
				}
				locationRow.push_back( uniformLocation );
			}
			locationColumn.push_back( locationRow );
		}
		locationMatrix.push_back( locationColumn );
	}

	// Finally, figure out the uniform memory layout for materials.
	uniformMemoryOffsets.clear();
	uniformMemoryOffsets.reserve( uniformCount );
	u32 position = 0;
	for ( u32 i = 0; i < uniformCount ; i++ ) {
		uniformMemoryOffsets.push_back( position );
		position += uniformTypeStrides.at(uniformTypes[i]) * uniformWidths[i];
	}
	uniformBufferSize = position;
}

Shader::Shader( const std::unordered_map<std::string,ShaderSource> &sources ) {
	u32 i = 0;
	for ( auto &pair : sources ) {
		auto &name = pair.first;
		auto &src = pair.second;
		passes.push_back( ShaderPass( src, {} ) );
		passMap[name] = i++;
	}
	passCount = passes.size();
	BuildUniformData();
}
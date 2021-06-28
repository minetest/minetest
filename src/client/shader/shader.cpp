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

void Shader::BuildUniformData() {

	std::unordered_set<std::string> alreadyWarned;
	std::unordered_map<std::string,u32> allUniformTypes;
	std::unordered_map<std::string,u32> allArrayLengths;

	// Grand uniform matrix in a yet-unresolved form.
	std::vector<std::vector<std::unordered_map<std::string,ProgramUniform> > > allProgramUniforms;

	for ( auto &pass : passes ) {
		std::vector<std::vector<ProgramUniform> > passUniforms;
		for ( int i = 0; i < pass.GetVariantCount(); i++ ) {
			std::vector<ProgramUniform> programUniforms;
			auto &program = pass.GetProgram( i );
			programUniforms = program.GetUniforms();
			for ( auto &uniform : programUniforms ) {
				if ( !STL_CONTAINS( allUniformTypes, uniform.first ) ) {
					allUniformTypes[uniform.first] = uniform.second.type;
					allArrayLengths[uniform.first] = uniform.second.arrayLength;
				} else {
					if ( allUniformTypes[uniform.first] != uniform.second.type
						&& !STL_CONTAINS( alreadyWarned, uniform.first ) ) {
						warningstream << "Type of uniform " << uniform.first
							<< " is not the same between passes or variants."
							<< " This is unsupported and may result"
							<< " in undefined behavior.\n";
						alreadyWarned.insert(uniform.first);
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
		uniformArrayLengths.push_back( allArrayLengths.at(pair.first) );
		uniformIndexMap[pair.first] = uniformCount;
		uniformCount++;
	}

	// Iterate over all uniforms again.
	for ( int i = 0; i < passes.size(); i++ ) {
		auto &pass = passes[i];
		std::vector<std::vector<s32> > locationColumn;
		for ( int j = 0; j < pass.GetVariantCount(); j++ ) {
			std::vector<s32> locationRow;
			auto &uniforms = allProgramUniforms[i][j];
			for ( int k = 0; k < uniformTypes.size(); k++ ) {
				s32 uniformLocation;
				if ( STL_CONTAINS( allProgramUniforms[i][j], uniformNames[k] ) ) {
					// The uniform with this name has the following location in this program:
					uniformLocation = allProgramUniforms[i][j].at( uniformNames[k] ).location;
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
		position += uniformTypeStrides[uniformTypes[i]];
	}
	uniformMemorySize = position;
}

Shader::Shader( const std::unordered_map<std::string,PassSources> &sources ) {
	for ( auto &pair : sources ) {
		auto &name = pair.first;
		auto &src = pair.second;

	}
	BuildUniformData();
}
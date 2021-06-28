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

#include "client/shader/shader_program.h"

// Is 'ext' an extension supported by the current device?
bool ExtensionPresent( const std::string &ext ) {
	return true; // dummy
}

ShaderProgram::ShaderProgram( PassSources sources, std::string extraHeader = "" ) {
	u32 stages[5];
	handle = glCreateProgram();

	if ( sources.stages[static_cast<u32>(StageIndex::Vertex)].length() == 0 ) {
		errorstream << "Vertex shader source missing.";
		return;
	}
	if ( sources.stages[static_cast<u32>(StageIndex::Fragment)].length() == 0 ) {
		errorstream << "Fragment shader source missing.";
		return;
	}

	for ( int i = 0; i < 5; i++ ) {
		if ( sources.stages[i].length() > 0 && ExtensionPresent( stageExtension[i] ) ) {
			stages[i] = glCreateShader( stageTypes[i] );
			if ( glIsShader( stages[i] ) ) {
				glShaderSource(
					stages[i], 3,
					{
						globalHeader.str(),
						sources.header.c_str(),
						sources.stages[i].c_str()
					},
					{ -1, -1, -1 } ); // C-strings need no lengths.
				glCompileShader( stages[i] );
				glGetShaderiv( stages[i], GL_COMPILE_STATUS, &compileStatus[i] );
			}
			if ( compileStatus[i] == StageCompileStatus::Success ) {
				glAttachShader( handle, stages[i] );
			}
		} else {
			compileStatus[i] = StageCompileStatus::NoSource;
		}
	}

	glLinkProgram( programHandle );
	glGetProgramiv( programHandle, GL_LINK_STATUS, &linked );



	// Regardless of how linking went, the shaders can be discarded now.
	for (int i = 0; i < 5; i++ ) {
		if ( stages[i] ) {
			if ( programHandle )
				glDetachShader( programHandle, stages[i] );
			glDeleteShader( stages[i] );
		}
	}
}
std::unordered_map<std::string,ProgramUniform>ShaderProgram::GetUniforms() const {
	std::unordered_map<std::string,ProgramUniform> uniforms;
	if ( linked ) {
		u32 uniformCount;
		glGetProgramiv( programHandle, GL_ACTIVE_UNIFORMS, &uniformCount );

		u32 nameLength;
		glGetProgramiv( programHandle, ACTIVE_UNIFORM_MAX_LENGTH, &nameLength );

		u8 *nameBuffer = new u8[nameLength];
		for ( int i = 0; i < uniformCount ; i++ ) {
			u32 arrayLength;
			u32 type;
			u32 actualNameLength;
			glGetActiveUniform( programHandle, i, nameLength, &actualNameLength, &type, &arrayLength, nameBuffer );

			u32 location;
			glGetUniformLocation( programHandle, nameBuffer );

			uniforms[std::string( nameBuffer )] = {
				type,
				arrayLength,
				location,
			};
		}
	}
	return uniforms;
}

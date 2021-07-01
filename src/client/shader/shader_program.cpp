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
#include "log.h"

// Is 'ext' an extension supported by the current device?
bool ExtensionPresent( const std::string &ext ) {
	return true; // dummy
}
// Extensions that introduced each stage.
std::string stageExtensions[5] = {
	"ARB_shader_object",
	"ARB_tessellation_shader",
	"ARB_tessellation_shader",
	"ARB_geometry_shader"
	"ARB_shader_object",
};
// GL enums to select each stage.
const u32 stageTypes[5] = {
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,
};

ShaderProgram::ShaderProgram( const ShaderSource sources, const std::string extraHeader ) {
	u32 stages[5];
	programHandle = glCreateProgram();

	if ( sources.stages[static_cast<u32>(StageIndex::Vertex)].length() == 0 ) {
		errorstream << "Vertex shader source missing.";
		return;
	}
	if ( sources.stages[static_cast<u32>(StageIndex::Fragment)].length() == 0 ) {
		errorstream << "Fragment shader source missing.";
		return;
	}


	for ( u32 i = 0; i < 5; i++ ) {
		if ( sources.stages[i].length() > 0 && ExtensionPresent( stageExtensions[i] ) ) {
			stages[i] = glCreateShader( stageTypes[i] );
			if ( glIsShader( stages[i] ) ) {
				static const int lengths[3] = { -1, -1, -1 };
				const char* strings[] = {
					globalHeader.str().c_str(),
					sources.header.c_str(),
					extraHeader.c_str(),
					sources.stages[i].c_str(),
				};
				glShaderSource( stages[i], 3, strings, lengths );
				glCompileShader( stages[i] );
				GLint status;
				glGetShaderiv( stages[i], GL_COMPILE_STATUS, &status );
				compileStatus[i] = static_cast<StageCompileStatus>(status);
			}
			if ( compileStatus[i] == StageCompileStatus::Success ) {
				glAttachShader( programHandle, stages[i] );
			}
		} else {
			compileStatus[i] = StageCompileStatus::NoSource;
		}
	}

	glLinkProgram( programHandle );
	GLint linkStatus;
	glGetProgramiv( programHandle, GL_LINK_STATUS, &linkStatus );
	linked = static_cast<bool>(linkStatus);

	// Regardless of how linking went, the shaders can be discarded now.
	for ( u32 i = 0; i < 5; i++ ) {
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
		GLint uniformCount;
		glGetProgramiv( programHandle, GL_ACTIVE_UNIFORMS, &uniformCount );

		GLsizei nameLength;
		glGetProgramiv( programHandle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &nameLength );

		char *nameBuffer = new char[nameLength];
		for ( u32 i = 0; i < (u32)uniformCount ; i++ ) {
			s32 arrayLength;
			u32 type;
			GLsizei actualNameLength;
			glGetActiveUniform( programHandle, i, nameLength, &actualNameLength, &arrayLength, &type, nameBuffer );

			s32 location = glGetUniformLocation( programHandle, nameBuffer );

			if ( location > -1 ) {
				uniforms[std::string( nameBuffer )] = {
					type,
					(u32)arrayLength,
					(u32)location,
				};
			}
		}
	}
	return uniforms;
}

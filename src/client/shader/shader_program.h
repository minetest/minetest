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

struct ProgramUniform {
	u32				type;
	u32				arrayLength;
	s32				location;
};

// There are five shader stage types in OpenGL and there likely won't be more any soon.
// We order stages by their order of appearance in the pipeline; if all five are used
// by a program, they will be executed in this order.
enum class StageIndex : u8 {
	Vertex			= 0,
	TessControl		= 1,	// since GL4.0 or GLES3.2
	TessEval		= 2,
	Geometry		= 3,	// since GL3.2 or GLES3.2
	Fragment		= 4,
};
// Extensions that introduced each stage.
std::string stageExtensions[5] = {
	"ARB_shader_object",
	"ARB_tessellation_shader",
	"ARB_tessellation_shader",
	"ARB_geometry_shader"
	"ARB_shader_object",
};
// GL enums to select each stage.
u32 stageTypes[5] = {
	GL_VERTEX_SHADER,
	GL_TESS_CONTROL_SHADER,
	GL_TESS_EVALUATION_SHADER,
	GL_GEOMETRY_SHADER,
	GL_FRAGMENT_SHADER,
};
enum class StageCompileStatus : u32 {
	Failure = 0,	// Stage failed to compile.
	Success = 1,	// Stage compiled successfully.
	NoSource = 2,	// Stage source was not provided.
};

class ShaderProgram {
	/*
		An object encapsulating a single OpenGL program object.
		Refer to the OpenGL reference for details.

		For our purposes, a single ShaderProgram is equivalent
		to one variant of a given pass.
	*/

	StageCompileStatus compileStatus[5];
	std::string compileLog[5];

	// True only after successful linkage.
	bool linked;

	u32 programHandle;

	static u32 lastBoundProgram = 0;
public:
	// This blob of text prefixes every single shader source.
	// Use it to set platform specific or init-time defines.
	static std::stringstream globalHeader;

	inline StageCompileStatus GetCompileStatus( StageIndex i ) {
		return compileStatus[i];
	}
	inline bool IsLinked() { return linked; }

	inline void Bind() {
		// Temp kludge until state can be reduced in some smarter way.
		if ( programHandle != lastBoundProgram ) {
			glUseProgram( programHandle );
			lastBoundProgram = programHandle;
		}
	}

	std::unordered_map<std::string,ProgramUniform> GetUniforms() const;

	/* Programs are compiled and analyzed on construction. */
	ShaderProgram( PassSources sources );
	inline ~ShaderProgram() {
		glDeleteProgram( programHandle )
	}
};

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
	u32				location;
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

	static u32 lastBoundProgram;
public:
	// This blob of text prefixes every single shader source.
	// Use it to set platform specific or init-time defines.
	// (for example, the #version line)
	static std::stringstream globalHeader;

	inline const std::string GetCompileLog( StageIndex i ) {
		return compileLog[static_cast<u32>(i)];
	}

	inline StageCompileStatus GetCompileStatus( StageIndex i ) {
		return compileStatus[static_cast<u32>(i)];
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
	ShaderProgram( const ShaderSource sources, const std::string extraHeader = "" );

	inline ~ShaderProgram() {
		glDeleteProgram( programHandle );
	}
};

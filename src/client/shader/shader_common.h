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

#include "irrlichttypes.h"
#include "irr_v2d.h"
#include "irr_v3d.h"
#include <SColor.h>

#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <vector>
#include <string>
#include <cstring>
#include <sstream>

#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>

// Replacement for .contains(y) which requires c++20
#define STL_CONTAINS(x,y) ( x .find( y ) != x .end() )

// Return x[y] if present, or z
// Similar to nil-or short circuit in Lua
#define STL_AT_OR(x,y,z) ( ( x .find( y ) != x .end() ) ? x .at( y ) : z )

enum class CompareTest : u32 { // Shared by depth and stencil
	Never			= GL_NEVER,
	Always			= GL_ALWAYS,
	Less			= GL_LESS,
	LessEqual		= GL_LEQUAL,
	Equal			= GL_EQUAL,
	GreaterEqual	= GL_GEQUAL,
	Greater			= GL_GREATER,
	NotEqual		= GL_NOTEQUAL,
};
enum class BlendOp : u32 {
	Add				= GL_FUNC_ADD,
	Subtract		= GL_FUNC_SUBTRACT,
	RevSubtract		= GL_FUNC_REVERSE_SUBTRACT,
	Min				= GL_MIN,
	Max				= GL_MAX,
};
enum class BlendFactor : u32 {
	Zero			= GL_ZERO,
	One				= GL_ONE,
	Source			= GL_SRC_COLOR,
	Dest			= GL_DST_COLOR,
	SourceAlpha		= GL_SRC_ALPHA,
	DestAlpha		= GL_DST_ALPHA,
	NegSourceAlpha	= GL_ONE_MINUS_SRC_ALPHA,
	NegDestAlpha	= GL_ONE_MINUS_DST_ALPHA,
	NegSource		= GL_ONE_MINUS_SRC_COLOR,
	NegDest			= GL_ONE_MINUS_DST_COLOR,
};
enum class StencilOp : u32 {
	Keep			= GL_KEEP,
	Zero			= GL_ZERO,
	Replace			= GL_REPLACE,
	Increment		= GL_INCR,
	IncrementWrap	= GL_INCR_WRAP,
	Decrement		= GL_DECR,
	DecrementWrap	= GL_DECR_WRAP,
	Invert			= GL_INVERT,
};

// Non-programmable state of a shader or material.
// This struct is simple enough to allow easy hashing and equality comparison.
struct FixedFunctionState {
	static FixedFunctionState currentState;

	bool		useBlending = false;
	BlendOp		blendEquationColor;
	BlendOp		blendEquationAlpha;
	BlendFactor	srcBlendColor;
	BlendFactor	srcBlendAlpha;
	BlendFactor	dstBlendColor;
	BlendFactor	dstBlendAlpha;
	u32			blendConstantColor = 0xFFFFFFFF; // ARGB-8888

	// Alpha clip/test.
	// Note that it must be emulated with discard on some obscure drivers,
	// and thus we have to add a variant for that.
	bool		alphaTest = false;

	// See GLES 2.0 reference 4.1.4 "Stencil Test"
	bool		useStencilTest = false;
	CompareTest stencilFrontFunc;
	StencilOp	stencilFrontFail;
	StencilOp	stencilFrontZFail;
	StencilOp	stencilFrontPass;
	u32			stencilFrontRef;
	u32			stencilFrontMask;
	u32			stencilFrontWriteMask = 0xFFFFFFFF;
	CompareTest stencilBackFunc;
	StencilOp	stencilBackFail;
	StencilOp	stencilBackZFail;
	StencilOp	stencilBackPass;
	u32			stencilBackRef;
	u32			stencilBackMask;
	u32			stencilBackWriteMask = 0xFFFFFFFF;

	bool		useDepthTest = true;
	CompareTest	depthFunc = GL_LEQUAL;
	bool		depthWrite = true; // Disable for transparent shaders.

	inline void SetConstantBlendColor( const irr::video::SColor &col ) {
		col.getData( &blendConstantColor, ECOLOR_FORMAT(irr::video::A8R8G8B8) )
	}

	void Set();
};


struct ShaderSource {
	std::string header; // Common header pasted right above each shader stage.

	std::string stages[5];
};

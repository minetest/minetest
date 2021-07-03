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

#include "shader_common.h"

static int antiAliasState = 0;

void FixedFunctionState::Set() {
	if ( std::memcmp(this, &currentState, sizeof( FixedFunctionState ) ) )
		return;

	// Is it faster to check a static variable than it is to call the driver
	// with a state change and hope that it optimizes that out? Yes, yes it is.
	#define CHANGED(x) ( currentState. x != x )
	#define UPDATE(x) currentState. x = x
	#define SETENABLE(x,y) if( x ) glEnable( y ); else glDisable( y );

	if( CHANGED( useBlending ) ) {
		UPDATE( useBlending );
		SETENABLE( useBlending, GL_BLEND );
	}

	if( CHANGED( blendEquationColor ) || CHANGED( blendEquationAlpha ) ) {
		UPDATE( blendEquationColor );
		UPDATE( blendEquationAlpha );
		glBlendEquationSeparate( blendEquationColor, blendEquationAlpha );
	}

	if ( CHANGED(srcBlendColor) || CHANGED(srcBlendAlpha) || CHANGED(dstBlendColor) || CHANGED(dstBlendAlpha) ) {
		UPDATE( srcBlendColor );
		UPDATE( srcBlendAlpha );
		UPDATE( dstBlendColor );
		UPDATE( dstBlendAlpha );
		glBlendFuncSeparate( srcBlendColor, dstBlendColor, srcBlendAlpha, dstBlendAlpha );
	}

	if ( CHANGED( blendConstantColor ) ) {
		UPDATE( blendConstantColor );
		glBlendColor(
			(GLclampf)( ( blendConstantColor & 0x000000FF ) >> 0  ) / 255.0f,
			(GLclampf)( ( blendConstantColor & 0xFF000000 ) >> 48 ) / 255.0f,
			(GLclampf)( ( blendConstantColor & 0x00FF0000 ) >> 32 ) / 255.0f,
			(GLclampf)( ( blendConstantColor & 0x0000FF00 ) >> 16 ) / 255.0f
		);
	}

	if ( CHANGED( alphaTest ) ) {
		UPDATE( alphaTest );
		SETENABLE( alphaTest, GL_ALPHA_TEST );
	}

	if ( CHANGED( stencilTest ) ) {
		UPDATE( stencilTest );
		SETENABLE( stencilTest, GL_STENCIL_TEST );
	}

	if ( CHANGED( stencilFrontFunc ) || CHANGED( stencilFrontRef ) || CHANGED( stencilFrontMask ) ) {
		UPDATE( stencilFrontFunc );
		UPDATE( stencilFrontRef );
		UPDATE( stencilFrontMask );
		glStencilFuncSeparate( GL_FRONT, stencilFrontFunc, stencilFrontRef, stencilFrontMask );
	}
	if ( CHANGED( stencilBackFunc ) || CHANGED( stencilBackRef ) || CHANGED( stencilBackMask ) ) {
		UPDATE( stencilBackFunc );
		UPDATE( stencilBackRef );
		UPDATE( stencilBackMask );
		glStencilFuncSeparate( GL_BACK, stencilBackFunc, stencilBackRef, stencilBackMask );
	}
	if ( CHANGED( stencilFrontFail ) || CHANGED( stencilFrontZFail ) || CHANGED( stencilFrontPass ) ) {
		UPDATE( stencilFrontFail );
		UPDATE( stencilFrontZFail );
		UPDATE( stencilFrontPass );
		glStencilOpSeparate( GL_FRONT, stencilFrontFail, stencilFrontZFail, stencilFrontPass );
	}
	if ( CHANGED( stencilBackFail ) || CHANGED( stencilBackZFail ) || CHANGED( stencilBackPass ) ) {
		UPDATE( stencilBackFail );
		UPDATE( stencilBackZFail );
		UPDATE( stencilBackPass );
		glStencilOpSeparate( GL_BACK, stencilBackFail, stencilBackZFail, stencilBackPass );
	}
	if ( CHANGED( stencilFrontWriteMask ) ) {
		UPDATE( stencilFrontWriteMask );
		glStencilMaskSeparate( GL_FRONT, stencilFrontWriteMask );
	}
	if ( CHANGED( stencilBackWriteMask ) ) {
		UPDATE( stencilBackWriteMask );
		glStencilMaskSeparate( GL_BACK, stencilBackWriteMask );
	}

	if ( CHANGED( useDepthTest ) ) {
		UPDATE( useDepthTest );
		SETENABLE( useDepthTest, GL_DEPTH_TEST );
	}
	if ( CHANGED( depthFunc ) ) {
		UPDATE( depthFunc );
		glDepthFunc( depthFunc );
	}
	if ( CHANGED( depthWrite ) ) {
		UPDATE( depthWrite );
		glDepthMask( depthWrite );
	}

	#undef CHANGED
	#undef UPDATE
	#undef SETENABLE
}
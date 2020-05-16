// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "IBurningShader.h"

#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

// compile flag for this file
#undef USE_ZBUFFER
#undef IPOL_Z
#undef CMP_Z
#undef WRITE_Z

#undef IPOL_W
#undef CMP_W
#undef WRITE_W

#undef SUBTEXEL
#undef INVERSE_W

#undef IPOL_C0
#undef IPOL_T0
#undef IPOL_T1

// define render case
#define SUBTEXEL
#define INVERSE_W

#define USE_ZBUFFER
#define IPOL_W
#define CMP_W
#define WRITE_W

#define IPOL_C0
#define IPOL_T0
#define IPOL_T1

// apply global override
#ifndef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
	#undef INVERSE_W
#endif

#ifndef SOFTWARE_DRIVER_2_SUBTEXEL
	#undef SUBTEXEL
#endif

#ifndef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	#undef IPOL_C0
#endif

#if !defined ( SOFTWARE_DRIVER_2_USE_WBUFFER ) && defined ( USE_ZBUFFER )
	#ifndef SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT
		#undef IPOL_W
	#endif
	#define IPOL_Z

	#ifdef CMP_W
		#undef CMP_W
		#define CMP_Z
	#endif

	#ifdef WRITE_W
		#undef WRITE_W
		#define WRITE_Z
	#endif

#endif

namespace irr
{

namespace video
{

class CTRGTextureLightMap2_M4 : public IBurningShader
{
public:

	//! constructor
	CTRGTextureLightMap2_M4(CBurningVideoDriver* driver);

	//! draws an indexed triangle list
	virtual void drawTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c );


private:
	void scanline_bilinear ();

	sScanConvertData scan;
	sScanLineData line;

};

//! constructor
CTRGTextureLightMap2_M4::CTRGTextureLightMap2_M4(CBurningVideoDriver* driver)
: IBurningShader(driver)
{
	#ifdef _DEBUG
	setDebugName("CTRGTextureLightMap2_M4");
	#endif
}



/*!
*/
void CTRGTextureLightMap2_M4::scanline_bilinear ()
{
	tVideoSample *dst;

#ifdef USE_ZBUFFER
	fp24 *z;
#endif

	s32 xStart;
	s32 xEnd;
	s32 dx;


#ifdef SUBTEXEL
	f32 subPixel;
#endif

#ifdef IPOL_Z
	f32 slopeZ;
#endif
#ifdef IPOL_W
	fp24 slopeW;
#endif
#ifdef IPOL_C0
	sVec4 slopeC;
#endif
#ifdef IPOL_T0
	sVec2 slopeT[BURNING_MATERIAL_MAX_TEXTURES];
#endif

	// apply top-left fill-convention, left
	xStart = core::ceil32( line.x[0] );
	xEnd = core::ceil32( line.x[1] ) - 1;

	dx = xEnd - xStart;

	if ( dx < 0 )
		return;

	// slopes
	const f32 invDeltaX = core::reciprocal_approxim ( line.x[1] - line.x[0] );

#ifdef IPOL_Z
	slopeZ = (line.z[1] - line.z[0]) * invDeltaX;
#endif
#ifdef IPOL_W
	slopeW = (line.w[1] - line.w[0]) * invDeltaX;
#endif
#ifdef IPOL_C0
	slopeC = (line.c[0][1] - line.c[0][0]) * invDeltaX;
#endif
#ifdef IPOL_T0
	slopeT[0] = (line.t[0][1] - line.t[0][0]) * invDeltaX;
#endif
#ifdef IPOL_T1
	slopeT[1] = (line.t[1][1] - line.t[1][0]) * invDeltaX;
#endif

#ifdef SUBTEXEL
	subPixel = ( (f32) xStart ) - line.x[0];
#ifdef IPOL_Z
	line.z[0] += slopeZ * subPixel;
#endif
#ifdef IPOL_W
	line.w[0] += slopeW * subPixel;
#endif
#ifdef IPOL_C0
	line.c[0][0] += slopeC * subPixel;
#endif
#ifdef IPOL_T0
	line.t[0][0] += slopeT[0] * subPixel;
#endif
#ifdef IPOL_T1
	line.t[1][0] += slopeT[1] * subPixel;
#endif
#endif

	dst = (tVideoSample*)RenderTarget->lock() + ( line.y * RenderTarget->getDimension().Width ) + xStart;

#ifdef USE_ZBUFFER
	z = (fp24*) DepthBuffer->lock() + ( line.y * RenderTarget->getDimension().Width ) + xStart;
#endif


#ifdef INVERSE_W
	f32 inversew;
#endif

	tFixPoint tx0, tx1;
	tFixPoint ty0, ty1;

	tFixPoint r0, g0, b0;
	tFixPoint r1, g1, b1;
	tFixPoint r2, g2, b2;

#ifdef IPOL_C0
	tFixPoint r3, g3, b3;
#endif

	for ( s32 i = 0; i <= dx; i++ )
	{
#ifdef CMP_Z
		if ( line.z[0] < z[i] )
#endif
#ifdef CMP_W
		if ( line.w[0] >= z[i] )
#endif
		{
#ifdef INVERSE_W
			inversew = fix_inverse32 ( line.w[0] );

			tx0 = tofix ( line.t[0][0].x,inversew);
			ty0 = tofix ( line.t[0][0].y,inversew);
			tx1 = tofix ( line.t[1][0].x,inversew);
			ty1 = tofix ( line.t[1][0].y,inversew);

#ifdef IPOL_C0
			r3 = tofix ( line.c[0][0].y ,inversew );
			g3 = tofix ( line.c[0][0].z ,inversew );
			b3 = tofix ( line.c[0][0].w ,inversew );
#endif

#else
			tx0 = tofix ( line.t[0][0].x );
			ty0 = tofix ( line.t[0][0].y );
			tx1 = tofix ( line.t[1][0].x );
			ty1 = tofix ( line.t[1][0].y );

#ifdef IPOL_C0
			r3 = tofix ( line.c[0][0].y );
			g3 = tofix ( line.c[0][0].z );
			b3 = tofix ( line.c[0][0].w );
#endif

#endif
			getSample_texture ( r0, g0, b0, &IT[0], tx0, ty0 );
			getSample_texture ( r1, g1, b1, &IT[1], tx1, ty1 );

#ifdef IPOL_C0
			r2 = imulFix ( r0, r3 );
			g2 = imulFix ( g0, g3 );
			b2 = imulFix ( b0, b3 );

			r2 = clampfix_maxcolor ( imulFix_tex4 ( r2, r1 ) );
			g2 = clampfix_maxcolor ( imulFix_tex4 ( g2, g1 ) );
			b2 = clampfix_maxcolor ( imulFix_tex4 ( b2, b1 ) );
/*
			r2 = r3 << 8;
			g2 = g3 << 8;
			b2 = b3 << 8;
*/
#else
			r2 = clampfix_maxcolor ( imulFix_tex4 ( r0, r1 ) );
			g2 = clampfix_maxcolor ( imulFix_tex4 ( g0, g1 ) );
			b2 = clampfix_maxcolor ( imulFix_tex4 ( b0, b1 ) );
#endif


			dst[i] = fix_to_color ( r2, g2, b2 );

#ifdef WRITE_Z
			z[i] = line.z[0];
#endif
#ifdef WRITE_W
			z[i] = line.w[0];
#endif
		}

#ifdef IPOL_Z
		line.z[0] += slopeZ;
#endif
#ifdef IPOL_W
		line.w[0] += slopeW;
#endif
#ifdef IPOL_C0
		line.c[0][0] += slopeC;
#endif
#ifdef IPOL_T0
		line.t[0][0] += slopeT[0];
#endif
#ifdef IPOL_T1
		line.t[1][0] += slopeT[1];
#endif
	}

}

void CTRGTextureLightMap2_M4::drawTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c )
{
	// sort on height, y
	if ( F32_A_GREATER_B ( a->Pos.y , b->Pos.y ) ) swapVertexPointer(&a, &b);
	if ( F32_A_GREATER_B ( b->Pos.y , c->Pos.y ) ) swapVertexPointer(&b, &c);
	if ( F32_A_GREATER_B ( a->Pos.y , b->Pos.y ) ) swapVertexPointer(&a, &b);

	const f32 ca = c->Pos.y - a->Pos.y;
	const f32 ba = b->Pos.y - a->Pos.y;
	const f32 cb = c->Pos.y - b->Pos.y;
	// calculate delta y of the edges
	scan.invDeltaY[0] = core::reciprocal( ca );
	scan.invDeltaY[1] = core::reciprocal( ba );
	scan.invDeltaY[2] = core::reciprocal( cb );

	if ( F32_LOWER_0 ( scan.invDeltaY[0] )  )
		return;

	// find if the major edge is left or right aligned
	f32 temp[4];

	temp[0] = a->Pos.x - c->Pos.x;
	temp[1] = -ca;
	temp[2] = b->Pos.x - a->Pos.x;
	temp[3] = ba;

	scan.left = ( temp[0] * temp[3] - temp[1] * temp[2] ) > 0.f ? 0 : 1;
	scan.right = 1 - scan.left;

	// calculate slopes for the major edge
	scan.slopeX[0] = (c->Pos.x - a->Pos.x) * scan.invDeltaY[0];
	scan.x[0] = a->Pos.x;

#ifdef IPOL_Z
	scan.slopeZ[0] = (c->Pos.z - a->Pos.z) * scan.invDeltaY[0];
	scan.z[0] = a->Pos.z;
#endif

#ifdef IPOL_W
	scan.slopeW[0] = (c->Pos.w - a->Pos.w) * scan.invDeltaY[0];
	scan.w[0] = a->Pos.w;
#endif

#ifdef IPOL_C0
	scan.slopeC[0][0] = (c->Color[0] - a->Color[0]) * scan.invDeltaY[0];
	scan.c[0][0] = a->Color[0];
#endif

#ifdef IPOL_T0
	scan.slopeT[0][0] = (c->Tex[0] - a->Tex[0]) * scan.invDeltaY[0];
	scan.t[0][0] = a->Tex[0];
#endif

#ifdef IPOL_T1
	scan.slopeT[1][0] = (c->Tex[1] - a->Tex[1]) * scan.invDeltaY[0];
	scan.t[1][0] = a->Tex[1];
#endif

	// top left fill convention y run
	s32 yStart;
	s32 yEnd;

#ifdef SUBTEXEL
	f32 subPixel;
#endif


	// rasterize upper sub-triangle
	//if ( (f32) 0.0 != scan.invDeltaY[1]  )
	if ( F32_GREATER_0 ( scan.invDeltaY[1] )  )
	{
		// calculate slopes for top edge
		scan.slopeX[1] = (b->Pos.x - a->Pos.x) * scan.invDeltaY[1];
		scan.x[1] = a->Pos.x;

#ifdef IPOL_Z
		scan.slopeZ[1] = (b->Pos.z - a->Pos.z) * scan.invDeltaY[1];
		scan.z[1] = a->Pos.z;
#endif

#ifdef IPOL_W
		scan.slopeW[1] = (b->Pos.w - a->Pos.w) * scan.invDeltaY[1];
		scan.w[1] = a->Pos.w;
#endif

#ifdef IPOL_C0
		scan.slopeC[0][1] = (b->Color[0] - a->Color[0]) * scan.invDeltaY[1];
		scan.c[0][1] = a->Color[0];
#endif

#ifdef IPOL_T0
		scan.slopeT[0][1] = (b->Tex[0] - a->Tex[0]) * scan.invDeltaY[1];
		scan.t[0][1] = a->Tex[0];
#endif

#ifdef IPOL_T1
		scan.slopeT[1][1] = (b->Tex[1] - a->Tex[1]) * scan.invDeltaY[1];
		scan.t[1][1] = a->Tex[1];
#endif

		// apply top-left fill convention, top part
		yStart = core::ceil32( a->Pos.y );
		yEnd = core::ceil32( b->Pos.y ) - 1;

#ifdef SUBTEXEL
		subPixel = ( (f32) yStart ) - a->Pos.y;

		// correct to pixel center
		scan.x[0] += scan.slopeX[0] * subPixel;
		scan.x[1] += scan.slopeX[1] * subPixel;		

#ifdef IPOL_Z
		scan.z[0] += scan.slopeZ[0] * subPixel;
		scan.z[1] += scan.slopeZ[1] * subPixel;		
#endif

#ifdef IPOL_W
		scan.w[0] += scan.slopeW[0] * subPixel;
		scan.w[1] += scan.slopeW[1] * subPixel;		
#endif

#ifdef IPOL_C0
		scan.c[0][0] += scan.slopeC[0][0] * subPixel;
		scan.c[0][1] += scan.slopeC[0][1] * subPixel;		
#endif

#ifdef IPOL_T0
		scan.t[0][0] += scan.slopeT[0][0] * subPixel;
		scan.t[0][1] += scan.slopeT[0][1] * subPixel;		
#endif

#ifdef IPOL_T1
		scan.t[1][0] += scan.slopeT[1][0] * subPixel;
		scan.t[1][1] += scan.slopeT[1][1] * subPixel;		
#endif

#endif

		// rasterize the edge scanlines
		for( line.y = yStart; line.y <= yEnd; ++line.y)
		{
			line.x[scan.left] = scan.x[0];
			line.x[scan.right] = scan.x[1];

#ifdef IPOL_Z
			line.z[scan.left] = scan.z[0];
			line.z[scan.right] = scan.z[1];
#endif

#ifdef IPOL_W
			line.w[scan.left] = scan.w[0];
			line.w[scan.right] = scan.w[1];
#endif

#ifdef IPOL_C0
			line.c[0][scan.left] = scan.c[0][0];
			line.c[0][scan.right] = scan.c[0][1];
#endif

#ifdef IPOL_T0
			line.t[0][scan.left] = scan.t[0][0];
			line.t[0][scan.right] = scan.t[0][1];
#endif

#ifdef IPOL_T1
			line.t[1][scan.left] = scan.t[1][0];
			line.t[1][scan.right] = scan.t[1][1];
#endif

			// render a scanline
			scanline_bilinear ();

			scan.x[0] += scan.slopeX[0];
			scan.x[1] += scan.slopeX[1];

#ifdef IPOL_Z
			scan.z[0] += scan.slopeZ[0];
			scan.z[1] += scan.slopeZ[1];
#endif

#ifdef IPOL_W
			scan.w[0] += scan.slopeW[0];
			scan.w[1] += scan.slopeW[1];
#endif

#ifdef IPOL_C0
			scan.c[0][0] += scan.slopeC[0][0];
			scan.c[0][1] += scan.slopeC[0][1];
#endif

#ifdef IPOL_T0
			scan.t[0][0] += scan.slopeT[0][0];
			scan.t[0][1] += scan.slopeT[0][1];
#endif

#ifdef IPOL_T1
			scan.t[1][0] += scan.slopeT[1][0];
			scan.t[1][1] += scan.slopeT[1][1];
#endif

		}
	}

	// rasterize lower sub-triangle
	//if ( (f32) 0.0 != scan.invDeltaY[2] )
	if ( F32_GREATER_0 ( scan.invDeltaY[2] )  )
	{
		// advance to middle point
		//if( (f32) 0.0 != scan.invDeltaY[1] )
		if ( F32_GREATER_0 ( scan.invDeltaY[1] )  )
		{
			temp[0] = b->Pos.y - a->Pos.y;	// dy

			scan.x[0] = a->Pos.x + scan.slopeX[0] * temp[0];
#ifdef IPOL_Z
			scan.z[0] = a->Pos.z + scan.slopeZ[0] * temp[0];
#endif
#ifdef IPOL_W
			scan.w[0] = a->Pos.w + scan.slopeW[0] * temp[0];
#endif
#ifdef IPOL_C0
			scan.c[0][0] = a->Color[0] + scan.slopeC[0][0] * temp[0];
#endif
#ifdef IPOL_T0
			scan.t[0][0] = a->Tex[0] + scan.slopeT[0][0] * temp[0];
#endif
#ifdef IPOL_T1
			scan.t[1][0] = a->Tex[1] + scan.slopeT[1][0] * temp[0];
#endif

		}

		// calculate slopes for bottom edge
		scan.slopeX[1] = (c->Pos.x - b->Pos.x) * scan.invDeltaY[2];
		scan.x[1] = b->Pos.x;

#ifdef IPOL_Z
		scan.slopeZ[1] = (c->Pos.z - b->Pos.z) * scan.invDeltaY[2];
		scan.z[1] = b->Pos.z;
#endif

#ifdef IPOL_W
		scan.slopeW[1] = (c->Pos.w - b->Pos.w) * scan.invDeltaY[2];
		scan.w[1] = b->Pos.w;
#endif

#ifdef IPOL_C0
		scan.slopeC[0][1] = (c->Color[0] - b->Color[0]) * scan.invDeltaY[2];
		scan.c[0][1] = b->Color[0];
#endif

#ifdef IPOL_T0
		scan.slopeT[0][1] = (c->Tex[0] - b->Tex[0]) * scan.invDeltaY[2];
		scan.t[0][1] = b->Tex[0];
#endif

#ifdef IPOL_T1
		scan.slopeT[1][1] = (c->Tex[1] - b->Tex[1]) * scan.invDeltaY[2];
		scan.t[1][1] = b->Tex[1];
#endif

		// apply top-left fill convention, top part
		yStart = core::ceil32( b->Pos.y );
		yEnd = core::ceil32( c->Pos.y ) - 1;

#ifdef SUBTEXEL

		subPixel = ( (f32) yStart ) - b->Pos.y;

		// correct to pixel center
		scan.x[0] += scan.slopeX[0] * subPixel;
		scan.x[1] += scan.slopeX[1] * subPixel;		

#ifdef IPOL_Z
		scan.z[0] += scan.slopeZ[0] * subPixel;
		scan.z[1] += scan.slopeZ[1] * subPixel;		
#endif

#ifdef IPOL_W
		scan.w[0] += scan.slopeW[0] * subPixel;
		scan.w[1] += scan.slopeW[1] * subPixel;		
#endif

#ifdef IPOL_C0
		scan.c[0][0] += scan.slopeC[0][0] * subPixel;
		scan.c[0][1] += scan.slopeC[0][1] * subPixel;		
#endif

#ifdef IPOL_T0
		scan.t[0][0] += scan.slopeT[0][0] * subPixel;
		scan.t[0][1] += scan.slopeT[0][1] * subPixel;		
#endif

#ifdef IPOL_T1
		scan.t[1][0] += scan.slopeT[1][0] * subPixel;
		scan.t[1][1] += scan.slopeT[1][1] * subPixel;		
#endif

#endif

		// rasterize the edge scanlines
		for( line.y = yStart; line.y <= yEnd; ++line.y)
		{
			line.x[scan.left] = scan.x[0];
			line.x[scan.right] = scan.x[1];

#ifdef IPOL_Z
			line.z[scan.left] = scan.z[0];
			line.z[scan.right] = scan.z[1];
#endif

#ifdef IPOL_W
			line.w[scan.left] = scan.w[0];
			line.w[scan.right] = scan.w[1];
#endif

#ifdef IPOL_C0
			line.c[0][scan.left] = scan.c[0][0];
			line.c[0][scan.right] = scan.c[0][1];
#endif

#ifdef IPOL_T0
			line.t[0][scan.left] = scan.t[0][0];
			line.t[0][scan.right] = scan.t[0][1];
#endif

#ifdef IPOL_T1
			line.t[1][scan.left] = scan.t[1][0];
			line.t[1][scan.right] = scan.t[1][1];
#endif

			// render a scanline
			scanline_bilinear ();

			scan.x[0] += scan.slopeX[0];
			scan.x[1] += scan.slopeX[1];

#ifdef IPOL_Z
			scan.z[0] += scan.slopeZ[0];
			scan.z[1] += scan.slopeZ[1];
#endif

#ifdef IPOL_W
			scan.w[0] += scan.slopeW[0];
			scan.w[1] += scan.slopeW[1];
#endif

#ifdef IPOL_C0
			scan.c[0][0] += scan.slopeC[0][0];
			scan.c[0][1] += scan.slopeC[0][1];
#endif

#ifdef IPOL_T0
			scan.t[0][0] += scan.slopeT[0][0];
			scan.t[0][1] += scan.slopeT[0][1];
#endif

#ifdef IPOL_T1
			scan.t[1][0] += scan.slopeT[1][0];
			scan.t[1][1] += scan.slopeT[1][1];
#endif

		}
	}

}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_

namespace irr
{
namespace video
{


//! creates a flat triangle renderer
IBurningShader* createTriangleRendererGTextureLightMap2_M4(CBurningVideoDriver* driver)
{
	#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_
	return new CTRGTextureLightMap2_M4(driver);
	#else
	return 0;
	#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
}


} // end namespace video
} // end namespace irr




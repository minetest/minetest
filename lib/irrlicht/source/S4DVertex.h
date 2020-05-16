// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h


#ifndef __S_4D_VERTEX_H_INCLUDED__
#define __S_4D_VERTEX_H_INCLUDED__

#include "SoftwareDriver2_compile_config.h"
#include "SoftwareDriver2_helper.h"
#include "irrAllocator.h"

namespace irr
{

namespace video
{

struct sVec2
{
	f32 x;
	f32 y;

	sVec2 () {}

	sVec2 ( f32 s) : x ( s ), y ( s ) {}
	sVec2 ( f32 _x, f32 _y )
		: x ( _x ), y ( _y ) {}

	void set ( f32 _x, f32 _y )
	{
		x = _x;
		y = _y;
	}

	// f = a * t + b * ( 1 - t )
	void interpolate(const sVec2& a, const sVec2& b, const f32 t)
	{
		x = b.x + ( ( a.x - b.x ) * t );
		y = b.y + ( ( a.y - b.y ) * t );
	}

	sVec2 operator-(const sVec2& other) const
	{
		return sVec2(x - other.x, y - other.y);
	}

	sVec2 operator+(const sVec2& other) const
	{
		return sVec2(x + other.x, y + other.y);
	}

	void operator+=(const sVec2& other)
	{
		x += other.x;
		y += other.y;
	}

	sVec2 operator*(const f32 s) const
	{
		return sVec2(x * s , y * s);
	}

	void operator*=( const f32 s)
	{
		x *= s;
		y *= s;
	}

	void operator=(const sVec2& other)
	{
		x = other.x;
		y = other.y;
	}

};

// A8R8G8B8
struct sVec4;
struct sCompressedVec4
{
	u32 argb;

	void setA8R8G8B8 ( u32 value )
	{
		argb = value;
	}

	void setColorf ( const video::SColorf & color )
	{
		argb = 	core::floor32 ( color.a * 255.f ) << 24 |
				core::floor32 ( color.r * 255.f ) << 16 |
				core::floor32 ( color.g * 255.f ) << 8  |
				core::floor32 ( color.b * 255.f );
	}

	void setVec4 ( const sVec4 & v );

	// f = a * t + b * ( 1 - t )
	void interpolate(const sCompressedVec4& a, const sCompressedVec4& b, const f32 t)
	{
		argb = PixelBlend32 ( b.argb, a.argb, core::floor32 ( t * 256.f ) );
	}


};


struct sVec4
{
	union
	{
		struct { f32 x, y, z, w; };
		struct { f32 a, r, g, b; };
//		struct { sVec2 xy, zw; };	// sorry, this does not compile with gcc
	};



	sVec4 () {}

	sVec4 ( f32 s) : x ( s ), y ( s ), z ( s ), w ( s ) {}

	sVec4 ( f32 _x, f32 _y, f32 _z, f32 _w )
		: x ( _x ), y ( _y ), z( _z ), w ( _w ){}

	void set ( f32 _x, f32 _y, f32 _z, f32 _w )
	{
		x = _x;
		y = _y;
		z = _z;
		w = _w;
	}

	void setA8R8G8B8 ( u32 argb )
	{
		x = ( ( argb & 0xFF000000 ) >> 24 ) * ( 1.f / 255.f );
		y = ( ( argb & 0x00FF0000 ) >> 16 ) * ( 1.f / 255.f );
		z = ( ( argb & 0x0000FF00 ) >>  8 ) * ( 1.f / 255.f );
		w = ( ( argb & 0x000000FF )       ) * ( 1.f / 255.f );
	}


	void setColorf ( const video::SColorf & color )
	{
		x = color.a;
		y = color.r;
		z = color.g;
		w = color.b;
	}


	// f = a * t + b * ( 1 - t )
	void interpolate(const sVec4& a, const sVec4& b, const f32 t)
	{
		x = b.x + ( ( a.x - b.x ) * t );
		y = b.y + ( ( a.y - b.y ) * t );
		z = b.z + ( ( a.z - b.z ) * t );
		w = b.w + ( ( a.w - b.w ) * t );
	}


	f32 dotProduct(const sVec4& other) const
	{
		return x*other.x + y*other.y + z*other.z + w*other.w;
	}

	f32 dot_xyz( const sVec4& other) const
	{
		return x*other.x + y*other.y + z*other.z;
	}

	f32 get_length_xyz_square () const
	{
		return x * x + y * y + z * z;
	}

	f32 get_length_xyz () const
	{
		return core::squareroot ( x * x + y * y + z * z );
	}

	void normalize_xyz ()
	{
		const f32 l = core::reciprocal_squareroot ( x * x + y * y + z * z );

		x *= l;
		y *= l;
		z *= l;
	}

	void project_xyz ()
	{
		w = core::reciprocal ( w );
		x *= w;
		y *= w;
		z *= w;
	}

	sVec4 operator-(const sVec4& other) const
	{
		return sVec4(x - other.x, y - other.y, z - other.z,w - other.w);
	}

	sVec4 operator+(const sVec4& other) const
	{
		return sVec4(x + other.x, y + other.y, z + other.z,w + other.w);
	}

	void operator+=(const sVec4& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		w += other.w;
	}

	sVec4 operator*(const f32 s) const
	{
		return sVec4(x * s , y * s, z * s,w * s);
	}

	sVec4 operator*(const sVec4 &other) const
	{
		return sVec4(x * other.x , y * other.y, z * other.z,w * other.w);
	}

	void mulReciprocal ( f32 s )
	{
		const f32 i = core::reciprocal ( s );
		x = (f32) ( x * i );
		y = (f32) ( y * i );
		z = (f32) ( z * i );
		w = (f32) ( w * i );
	}

	void mul ( const f32 s )
	{
		x *= s;
		y *= s;
		z *= s;
		w *= s;
	}

/*
	void operator*=(f32 s)
	{
		x *= s;
		y *= s;
		z *= s;
		w *= s;
	}
*/
	void operator*=(const sVec4 &other)
	{
		x *= other.x;
		y *= other.y;
		z *= other.z;
		w *= other.w;
	}

	void operator=(const sVec4& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}
};

struct sVec3
{
	union
	{
		struct { f32 r, g, b; };
		struct { f32 x, y, z; };
	};


	sVec3 () {}
	sVec3 ( f32 _x, f32 _y, f32 _z )
		: r ( _x ), g ( _y ), b( _z ) {}

	sVec3 ( const sVec4 &v )
		: r ( v.x ), g ( v.y ), b( v.z ) {}

	void set ( f32 _r, f32 _g, f32 _b )
	{
		r = _r;
		g = _g;
		b = _b;
	}

	void setR8G8B8 ( u32 argb )
	{
		r = ( ( argb & 0x00FF0000 ) >> 16 ) * ( 1.f / 255.f );
		g = ( ( argb & 0x0000FF00 ) >>  8 ) * ( 1.f / 255.f );
		b = ( ( argb & 0x000000FF )       ) * ( 1.f / 255.f );
	}

	void setColorf ( const video::SColorf & color )
	{
		r = color.r;
		g = color.g;
		b = color.b;
	}

	void add (const sVec3& other)
	{
		r += other.r;
		g += other.g;
		b += other.b;
	}

	void mulAdd(const sVec3& other, const f32 v)
	{
		r += other.r * v;
		g += other.g * v;
		b += other.b * v;
	}

	void mulAdd(const sVec3& v0, const sVec3& v1)
	{
		r += v0.r * v1.r;
		g += v0.g * v1.g;
		b += v0.b * v1.b;
	}

	void saturate ( sVec4 &dest, u32 argb )
	{
		dest.x = ( ( argb & 0xFF000000 ) >> 24 ) * ( 1.f / 255.f );
		dest.y = core::min_ ( r, 1.f );
		dest.z = core::min_ ( g, 1.f );
		dest.w = core::min_ ( b, 1.f );
	}

	// f = a * t + b * ( 1 - t )
	void interpolate(const sVec3& v0, const sVec3& v1, const f32 t)
	{
		r = v1.r + ( ( v0.r - v1.r ) * t );
		g = v1.g + ( ( v0.g - v1.g ) * t );
		b = v1.b + ( ( v0.b - v1.b ) * t );
	}

	sVec3 operator-(const sVec3& other) const
	{
		return sVec3(r - other.r, b - other.b, g - other.g);
	}

	sVec3 operator+(const sVec3& other) const
	{
		return sVec3(r + other.r, g + other.g, b + other.b);
	}

	sVec3 operator*(const f32 s) const
	{
		return sVec3(r * s , g * s, b * s);
	}

	sVec3 operator/(const f32 s) const
	{
		f32 inv = 1.f / s;
		return sVec3(r * inv , g * inv, b * inv);
	}

	sVec3 operator*(const sVec3 &other) const
	{
		return sVec3(r * other.r , b * other.b, g * other.g);
	}

	void operator+=(const sVec3& other)
	{
		r += other.r;
		g += other.g;
		b += other.b;
	}

	void setLength ( f32 len )
	{
		const f32 l = len * core::reciprocal_squareroot ( r * r + g * g + b * b );

		r *= l;
		g *= l;
		b *= l;
	}

};



inline void sCompressedVec4::setVec4 ( const sVec4 & v )
{
	argb = 	core::floor32 ( v.x * 255.f ) << 24 |
			core::floor32 ( v.y * 255.f ) << 16 |
			core::floor32 ( v.z * 255.f ) << 8  |
			core::floor32 ( v.w * 255.f );
}


enum e4DVertexFlag
{
	VERTEX4D_INSIDE		= 0x0000003F,
	VERTEX4D_CLIPMASK	= 0x0000003F,
	VERTEX4D_PROJECTED	= 0x00000100,

	VERTEX4D_FORMAT_MASK			= 0xFFFF0000,

	VERTEX4D_FORMAT_MASK_TEXTURE	= 0x000F0000,
	VERTEX4D_FORMAT_TEXTURE_1		= 0x00010000,
	VERTEX4D_FORMAT_TEXTURE_2		= 0x00020000,
	VERTEX4D_FORMAT_TEXTURE_3		= 0x00030000,
	VERTEX4D_FORMAT_TEXTURE_4		= 0x00040000,

	VERTEX4D_FORMAT_MASK_COLOR		= 0x00F00000,
	VERTEX4D_FORMAT_COLOR_1			= 0x00100000,
	VERTEX4D_FORMAT_COLOR_2			= 0x00200000,

	VERTEX4D_FORMAT_MASK_BUMP		= 0x0F000000,
	VERTEX4D_FORMAT_BUMP_DOT3		= 0x01000000,

};

const u32 MATERIAL_MAX_COLORS = 1;
const u32 BURNING_MATERIAL_MAX_TEXTURES = 2;
const u32 BURNING_MATERIAL_MAX_TANGENT = 1;

// dummy Vertex. used for calculation vertex memory size
struct s4DVertex_proxy
{
	u32 flag;
	sVec4 Pos;
	sVec2 Tex[BURNING_MATERIAL_MAX_TEXTURES];

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	sVec4 Color[MATERIAL_MAX_COLORS];
#endif

	sVec3 LightTangent[BURNING_MATERIAL_MAX_TANGENT];

};

#define SIZEOF_SVERTEX	64
#define SIZEOF_SVERTEX_LOG2	6

/*!
	Internal BurningVideo Vertex
*/
struct s4DVertex
{
	u32 flag;

	sVec4 Pos;
	sVec2 Tex[ BURNING_MATERIAL_MAX_TEXTURES ];

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	sVec4 Color[ MATERIAL_MAX_COLORS ];
#endif

	sVec3 LightTangent[BURNING_MATERIAL_MAX_TANGENT];

	//u8 fill [ SIZEOF_SVERTEX - sizeof (s4DVertex_proxy) ];

	// f = a * t + b * ( 1 - t )
	void interpolate(const s4DVertex& b, const s4DVertex& a, const f32 t)
	{
		u32 i;
		u32 size;

		Pos.interpolate ( a.Pos, b.Pos, t );

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
		size = (flag & VERTEX4D_FORMAT_MASK_COLOR) >> 20;
		for ( i = 0; i!= size; ++i )
		{
			Color[i].interpolate ( a.Color[i], b.Color[i], t );
		}
#endif

		size = (flag & VERTEX4D_FORMAT_MASK_TEXTURE) >> 16;
		for ( i = 0; i!= size; ++i )
		{
			Tex[i].interpolate ( a.Tex[i], b.Tex[i], t );
		}

		size = (flag & VERTEX4D_FORMAT_MASK_BUMP) >> 24;
		for ( i = 0; i!= size; ++i )
		{
			LightTangent[i].interpolate ( a.LightTangent[i], b.LightTangent[i], t );
		}

	}
};

// ----------------- Vertex Cache ---------------------------

struct SAlignedVertex
{
	SAlignedVertex ( u32 element, u32 aligned )
		: ElementSize ( element )
	{
		u32 byteSize = (ElementSize << SIZEOF_SVERTEX_LOG2 ) + aligned;
		mem = new u8 [ byteSize ];
		data = (s4DVertex*) mem;
	}

	virtual ~SAlignedVertex ()
	{
		delete [] mem;
	}

	s4DVertex *data;
	u8 *mem;
	u32 ElementSize;
};


// hold info for different Vertex Types
struct SVSize
{
	u32 Format;
	u32 Pitch;
	u32 TexSize;
};


// a cache info
struct SCacheInfo
{
	u32 index;
	u32 hit;
};

#define VERTEXCACHE_ELEMENT	16
#define VERTEXCACHE_MISS 0xFFFFFFFF
struct SVertexCache
{
	SVertexCache (): mem ( VERTEXCACHE_ELEMENT * 2, 128 ) {}

	SCacheInfo info[VERTEXCACHE_ELEMENT];


	// Transformed and lite, clipping state
	// + Clipped, Projected
	SAlignedVertex mem;

	// source
	const void* vertices;
	u32 vertexCount;

	const void* indices;
	u32 indexCount;
	u32 indicesIndex;

	u32 indicesRun;

	// primitives consist of x vertices
	u32 primitivePitch;

	u32 vType;		//E_VERTEX_TYPE
	u32 pType;		//scene::E_PRIMITIVE_TYPE
	u32 iType;		//E_INDEX_TYPE iType

};


// swap 2 pointer
REALINLINE void swapVertexPointer(const s4DVertex** v1, const s4DVertex** v2)
{
	const s4DVertex* b = *v1;
	*v1 = *v2;
	*v2 = b;
}


// ------------------------ Internal Scanline Rasterizer -----------------------------



// internal scan convert
struct sScanConvertData
{
	u8 left;			// major edge left/right
	u8 right;			// !left

	f32 invDeltaY[3];	// inverse edge delta y

	f32 x[2];			// x coordinate
	f32 slopeX[2];		// x slope along edges

#if defined ( SOFTWARE_DRIVER_2_USE_WBUFFER ) || defined ( SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT )
	f32 w[2];			// w coordinate
	fp24 slopeW[2];		// w slope along edges
#else
	f32 z[2];			// z coordinate
	f32 slopeZ[2];		// z slope along edges
#endif

	sVec4 c[MATERIAL_MAX_COLORS][2];			// color
	sVec4 slopeC[MATERIAL_MAX_COLORS][2];	// color slope along edges

	sVec2 t[BURNING_MATERIAL_MAX_TEXTURES][2];		// texture
	sVec2 slopeT[BURNING_MATERIAL_MAX_TEXTURES][2];	// texture slope along edges

	sVec3 l[BURNING_MATERIAL_MAX_TANGENT][2];		// Light Tangent
	sVec3 slopeL[BURNING_MATERIAL_MAX_TEXTURES][2];	// tanget slope along edges
};

// passed to scan Line
struct sScanLineData
{
	s32 y;				// y position of scanline
	f32 x[2];			// x start, x end of scanline

#if defined ( SOFTWARE_DRIVER_2_USE_WBUFFER ) || defined ( SOFTWARE_DRIVER_2_PERSPECTIVE_CORRECT )
	f32 w[2];			// w start, w end of scanline
#else
	f32 z[2];			// z start, z end of scanline
#endif

#ifdef SOFTWARE_DRIVER_2_USE_VERTEX_COLOR
	sVec4 c[MATERIAL_MAX_COLORS][2];			// color start, color end of scanline
#endif

	sVec2 t[BURNING_MATERIAL_MAX_TEXTURES][2];		// texture start, texture end of scanline
	sVec3 l[BURNING_MATERIAL_MAX_TANGENT][2];		// Light Tangent start, end
};

// passed to pixel Shader
struct sPixelShaderData
{
	tVideoSample *dst;
	fp24 *z;

	s32 xStart;
	s32 xEnd;
	s32 dx;
	s32 i;
};

/*
	load a color value
*/
inline void getTexel_plain2 (	tFixPoint &r, tFixPoint &g, tFixPoint &b,
							const sVec4 &v
							)
{
	r = tofix ( v.y );
	g = tofix ( v.z );
	b = tofix ( v.w );
}

/*
	load a color value
*/
inline void getSample_color (	tFixPoint &a, tFixPoint &r, tFixPoint &g, tFixPoint &b,
							const sVec4 &v
							)
{
	a = tofix ( v.x );
	r = tofix ( v.y, COLOR_MAX * FIX_POINT_F32_MUL);
	g = tofix ( v.z, COLOR_MAX * FIX_POINT_F32_MUL);
	b = tofix ( v.w, COLOR_MAX * FIX_POINT_F32_MUL);
}

/*
	load a color value
*/
inline void getSample_color ( tFixPoint &r, tFixPoint &g, tFixPoint &b,const sVec4 &v )
{
	r = tofix ( v.y, COLOR_MAX * FIX_POINT_F32_MUL);
	g = tofix ( v.z, COLOR_MAX * FIX_POINT_F32_MUL);
	b = tofix ( v.w, COLOR_MAX * FIX_POINT_F32_MUL);
}

/*
	load a color value
*/
inline void getSample_color (	tFixPoint &r, tFixPoint &g, tFixPoint &b,
								const sVec4 &v, const f32 mulby )
{
	r = tofix ( v.y, mulby);
	g = tofix ( v.z, mulby);
	b = tofix ( v.w, mulby);
}



}

}

#endif


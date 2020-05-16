// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_BURNING_SHADER_H_INCLUDED__
#define __I_BURNING_SHADER_H_INCLUDED__

#include "SoftwareDriver2_compile_config.h"
#include "IReferenceCounted.h"
#include "irrMath.h"
#include "IImage.h"
#include "S2DVertex.h"
#include "rect.h"
#include "CDepthBuffer.h"
#include "S4DVertex.h"
#include "irrArray.h"
#include "SLight.h"
#include "SMaterial.h"
#include "os.h"


namespace irr
{

namespace video
{

	struct SBurningShaderLight
	{
		//SLight org;
		bool LightIsOn;

		E_LIGHT_TYPE Type;
		f32 radius;
		f32 linearAttenuation;
		f32 constantAttenuation;
		f32 quadraticAttenuation;
		sVec4 pos;

		sVec3 AmbientColor;
		sVec3 DiffuseColor;
		sVec3 SpecularColor;
		sVec4 pos_objectspace;
	};

	enum eLightFlags
	{
		ENABLED		= 0x01,
		POINTLIGHT	= 0x02,
		SPECULAR	= 0x04,
		FOG			= 0x08,
		NORMALIZE	= 0x10,
		VERTEXTRANSFORM	= 0x20,
	};

	struct SBurningShaderLightSpace
	{
		void reset ()
		{
			Light.set_used ( 0 );
			Global_AmbientLight.set ( 0.f, 0.f, 0.f );
			Flags = 0;
		}
		core::array<SBurningShaderLight> Light;
		sVec3 Global_AmbientLight;
		sVec4 FogColor;
		sVec4 campos;
		sVec4 vertex;
		sVec4 normal;
		u32 Flags;
	};

	struct SBurningShaderMaterial
	{
		SMaterial org;

		sVec3 AmbientColor;
		sVec3 DiffuseColor;
		sVec3 SpecularColor;
		sVec3 EmissiveColor;

	};

	enum EBurningFFShader
	{
		ETR_FLAT = 0,
		ETR_FLAT_WIRE,
		ETR_GOURAUD,
		ETR_GOURAUD_WIRE,
		ETR_TEXTURE_FLAT,
		ETR_TEXTURE_FLAT_WIRE,
		ETR_TEXTURE_GOURAUD,
		ETR_TEXTURE_GOURAUD_WIRE,
		ETR_TEXTURE_GOURAUD_NOZ,
		ETR_TEXTURE_GOURAUD_ADD,
		ETR_TEXTURE_GOURAUD_ADD_NO_Z,

		ETR_TEXTURE_GOURAUD_VERTEX_ALPHA,

		ETR_TEXTURE_GOURAUD_LIGHTMAP_M1,
		ETR_TEXTURE_GOURAUD_LIGHTMAP_M2,
		ETR_TEXTURE_GOURAUD_LIGHTMAP_M4,
		ETR_TEXTURE_LIGHTMAP_M4,

		ETR_TEXTURE_GOURAUD_DETAIL_MAP,
		ETR_TEXTURE_GOURAUD_LIGHTMAP_ADD,
	
		ETR_GOURAUD_ALPHA,
		ETR_GOURAUD_ALPHA_NOZ,

		ETR_TEXTURE_GOURAUD_ALPHA,
		ETR_TEXTURE_GOURAUD_ALPHA_NOZ,

		ETR_NORMAL_MAP_SOLID,
		ETR_STENCIL_SHADOW,

		ETR_TEXTURE_BLEND,
		ETR_REFERENCE,
		ETR_INVALID,

		ETR2_COUNT
	};


	class CBurningVideoDriver;
	class IBurningShader : public virtual IReferenceCounted
	{
	public:
		IBurningShader(CBurningVideoDriver* driver);

		//! destructor
		virtual ~IBurningShader();

		//! sets a render target
		virtual void setRenderTarget(video::IImage* surface, const core::rect<s32>& viewPort);

		//! sets the Texture
		virtual void setTextureParam( u32 stage, video::CSoftwareTexture2* texture, s32 lodLevel);
		virtual void drawTriangle ( const s4DVertex *a,const s4DVertex *b,const s4DVertex *c ) = 0;
		virtual void drawLine ( const s4DVertex *a,const s4DVertex *b) {};

		virtual void setParam ( u32 index, f32 value) {};
		virtual void setZCompareFunc ( u32 func) {};

		virtual void setMaterial ( const SBurningShaderMaterial &material ) {};

	protected:

		CBurningVideoDriver *Driver;

		video::CImage* RenderTarget;
		CDepthBuffer* DepthBuffer;
		CStencilBuffer * Stencil;
		tVideoSample ColorMask;

		sInternalTexture IT[ BURNING_MATERIAL_MAX_TEXTURES ];

		static const tFixPointu dithermask[ 4 * 4];
	};


	IBurningShader* createTriangleRendererTextureGouraud2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_M1(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_M2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_M4(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGTextureLightMap2_M4(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureLightMap2_Add(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureDetailMap2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureVertexAlpha2(CBurningVideoDriver* driver);


	IBurningShader* createTriangleRendererTextureGouraudWire2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGouraud2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGouraudAlpha2(CBurningVideoDriver* driver);
	IBurningShader* createTRGouraudAlphaNoZ2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererGouraudWire2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureFlat2(CBurningVideoDriver* driver);
	IBurningShader* createTriangleRendererTextureFlatWire2(CBurningVideoDriver* driver);
	IBurningShader* createTRFlat2(CBurningVideoDriver* driver);
	IBurningShader* createTRFlatWire2(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudNoZ2(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudAdd2(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudAddNoZ2(CBurningVideoDriver* driver);

	IBurningShader* createTRTextureGouraudAlpha(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureGouraudAlphaNoZ(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureBlend(CBurningVideoDriver* driver);
	IBurningShader* createTRTextureInverseAlphaBlend(CBurningVideoDriver* driver);

	IBurningShader* createTRNormalMap(CBurningVideoDriver* driver);
	IBurningShader* createTRStencilShadow(CBurningVideoDriver* driver);

	IBurningShader* createTriangleRendererReference(CBurningVideoDriver* driver);



} // end namespace video
} // end namespace irr

#endif


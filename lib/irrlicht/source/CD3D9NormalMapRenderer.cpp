// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_

#include "CD3D9NormalMapRenderer.h"
#include "IVideoDriver.h"
#include "IMaterialRendererServices.h"
#include "os.h"
#include "SLight.h"

namespace irr
{
namespace video  
{

	// 1.1 Shaders with two lights and vertex based attenuation

	// Irrlicht Engine D3D9 render path normal map vertex shader
	const char D3D9_NORMAL_MAP_VSH[] = 
		";Irrlicht Engine 0.8 D3D9 render path normal map vertex shader\n"\
		"; c0-3: Transposed world matrix \n"\
		"; c8-11: Transposed worldViewProj matrix (Projection * View * World) \n"\
		"; c12: Light01 position \n"\
		"; c13: x,y,z: Light01 color; .w: 1/LightRadius² \n"\
		"; c14: Light02 position \n"\
		"; c15: x,y,z: Light02 color; .w: 1/LightRadius² \n"\
		"vs.1.1\n"\
		"dcl_position  v0              ; position \n"\
		"dcl_normal    v1              ; normal \n"\
		"dcl_color     v2              ; color \n"\
		"dcl_texcoord0 v3              ; texture coord \n"\
		"dcl_texcoord1 v4              ; tangent \n"\
		"dcl_texcoord2 v5              ; binormal \n"\
		"\n"\
		"def c95, 0.5, 0.5, 0.5, 0.5   ; used for moving light vector to ps \n"\
		"\n"\
		"m4x4 oPos, v0, c8             ; transform position to clip space with worldViewProj matrix\n"\
        "\n"\
		"m3x3 r5, v4, c0               ; transform tangent U\n"\
		"m3x3 r7, v1, c0               ; transform normal W\n"\
		"m3x3 r6, v5, c0               ; transform binormal V\n"\
		"\n"\
		"m4x4 r4, v0, c0               ; vertex into world position\n"\
		"add r2, c12, -r4              ; vtxpos - lightpos1\n"\
		"add r3, c14, -r4              ; vtxpos - lightpos2\n"\
		"\n"\
		"dp3 r8.x, r5, r2              ; transform the light vector 1 with U, V, W\n"\
		"dp3 r8.y, r6, r2   \n"\
		"dp3 r8.z, r7, r2   \n"\
		"dp3 r9.x, r5, r3              ; transform the light vector 2 with U, V, W\n"\
		"dp3 r9.y, r6, r3   \n"\
		"dp3 r9.z, r7, r3   \n"\
		"\n"\
		"dp3 r8.w, r8, r8              ; normalize light vector 1 (r8)\n"\
		"rsq r8.w, r8.w    \n"\
		"mul r8, r8, r8.w  \n"\
		"dp3 r9.w, r9, r9              ; normalize light vector 2 (r9)\n"\
		"rsq r9.w, r9.w    \n"\
		"mul r9, r9, r9.w  \n"\
		"\n"\
		"mad oT2.xyz, r8.xyz, c95, c95 ; move light vector 1 from -1..1 into 0..1 \n"\
		"mad oT3.xyz, r9.xyz, c95, c95 ; move light vector 2 from -1..1 into 0..1 \n"\
		"\n"\
		" ; calculate attenuation of light 1 \n"\
		"dp3 r2.x, r2.xyz, r2.xyz      ; r2.x = r2.x² + r2.y² + r2.z² \n"\
		"mul r2.x, r2.x, c13.w         ; r2.x * attenutation \n"\
		"rsq r2, r2.x                  ; r2.xyzw = 1/sqrt(r2.x * attenutation)\n"\
		"mul oD0, r2, c13              ; resulting light color = lightcolor * attenuation \n"\
		"\n"\
		" ; calculate attenuation of light 2 \n"\
		"dp3 r3.x, r3.xyz, r3.xyz      ; r3.x = r3.x² + r3.y² + r3.z² \n"\
		"mul r3.x, r3.x, c15.w         ; r2.x * attenutation \n"\
		"rsq r3, r3.x                  ; r2.xyzw = 1/sqrt(r2.x * attenutation)\n"\
		"mul oD1, r3, c15              ; resulting light color = lightcolor * attenuation \n"\
		"\n"\
		"mov oT0.xy, v3.xy             ; move out texture coordinates 1\n"\
		"mov oT1.xy, v3.xy             ; move out texture coordinates 2\n"\
		"mov oD0.a, v2.a               ; move out original alpha value \n"\
		"\n";

	// Irrlicht Engine D3D9 render path normal map pixel shader
	const char D3D9_NORMAL_MAP_PSH_1_1[] = 
		";Irrlicht Engine 0.8 D3D9 render path normal map pixel shader\n"\
		";Input: \n"\
		";t0: color map texture coord \n"\
		";t1: normal map texture coords \n"\
		";t2: light 1 vector in tangent space \n"\
		";v0: light 1 color \n"\
		";t3: light 2 vector in tangent space \n"\
		";v1: light 2 color \n"\
		";v0.a: vertex alpha value  \n"\
		"ps.1.1 \n"\
		"tex t0                     ; sample color map \n"\
		"tex t1                     ; sample normal map\n"\
		"texcoord t2     			; fetch light vector 1\n"\
		"texcoord t3     			; fetch light vector 2\n"\
		"\n"\
		"dp3_sat r0, t1_bx2, t2_bx2 ; normal dot light 1 (_bx2 because moved into 0..1)\n"\
		"mul r0, r0, v0             ; luminance1 * light color 1 \n"\
		"\n"\
		"dp3_sat r1, t1_bx2, t3_bx2 ; normal dot light 2 (_bx2 because moved into 0..1)\n"\
		"mad r0, r1, v1, r0         ; (luminance2 * light color 2) + luminance 1 \n"\
		"\n"\
		"mul r0.xyz, t0, r0             ; total luminance * base color\n"\
		"+mov r0.a, v0.a             ; write interpolated vertex alpha value \n"\
		"\n"\
		"";
		
	// Higher-quality normal map pixel shader (requires PS 2.0)
	// uses per-pixel normalization for improved accuracy
	const char D3D9_NORMAL_MAP_PSH_2_0[] = 
		";Irrlicht Engine 0.8 D3D9 render path normal map pixel shader\n"\
		";Input: \n"\
		";t0: color map texture coord \n"\
		";t1: normal map texture coords \n"\
		";t2: light 1 vector in tangent space \n"\
		";v0: light 1 color \n"\
		";t3: light 2 vector in tangent space \n"\
		";v1: light 2 color \n"\
		";v0.a: vertex alpha value  \n"\

		"ps_2_0 \n"\
		"def c0, 0, 0, 0, 0\n"\
		"def c1, 1.0, 1.0, 1.0, 1.0\n"\
		"def c2, 2.0, 2.0, 2.0, 2.0\n"\
		"def c3, -.5, -.5, -.5, -.5\n"\
		"dcl t0\n"\
		"dcl t1\n"\
		"dcl t2\n"\
		"dcl t3\n"\
		"dcl v1\n"\
		"dcl v0\n"\
		"dcl_2d s0\n"\
		"dcl_2d s1\n"\

		"texld r0, t0, s0			; sample color map into r0 \n"\
		"texld r4, t0, s1			; sample normal map into r4\n"\
		"add r4, r4, c3				; bias the normal vector\n"\
		"add r5, t2, c3				; bias the light 1 vector into r5\n"\
		"add r6, t3, c3				; bias the light 2 vector into r6\n"\

		"nrm r1, r4					; normalize the normal vector into r1\n"\
		"nrm r2, r5					; normalize the light1 vector into r2\n"\
		"nrm r3, r6					; normalize the light2 vector into r3\n"\
		
		"dp3 r2, r2, r1				; let r2 = normal DOT light 1 vector\n"\
		"max r2, r2, c0				; clamp result to positive numbers\n"\
		"mul r2, r2, v0             ; let r2 = luminance1 * light color 1 \n"\

		"dp3 r3, r3, r1				; let r3 = normal DOT light 2 vector\n"\
		"max r3, r3, c0				; clamp result to positive numbers\n"\

		"mad r2, r3, v1, r2         ; let r2 = (luminance2 * light color 2) + (luminance2 * light color 1) \n"\

		"mul r2, r2, r0	; let r2 = total luminance * base color\n"\
		"mov r2.w, v0.w				; write interpolated vertex alpha value \n"\

		"mov oC0, r2				; copy r2 to the output register \n"\

		"\n"\
		"";

	CD3D9NormalMapRenderer::CD3D9NormalMapRenderer(
		IDirect3DDevice9* d3ddev, video::IVideoDriver* driver, 
		s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial)
		: CD3D9ShaderMaterialRenderer(d3ddev, driver, 0, baseMaterial)
	{
		#ifdef _DEBUG
		setDebugName("CD3D9NormalMapRenderer");
		#endif
	
		// set this as callback. We could have done this in 
		// the initialization list, but some compilers don't like it.

		CallBack = this;

		// basically, this thing simply compiles the hardcoded shaders
		// if the hardware is able to do them, otherwise it maps to the
		// base material

		if (!driver->queryFeature(video::EVDF_PIXEL_SHADER_1_1) ||
			!driver->queryFeature(video::EVDF_VERTEX_SHADER_1_1))
		{
			// this hardware is not able to do shaders. Fall back to
			// base material.
			outMaterialTypeNr = driver->addMaterialRenderer(this);
			return;
		}

		// check if already compiled normal map shaders are there.

		video::IMaterialRenderer* renderer = driver->getMaterialRenderer(EMT_NORMAL_MAP_SOLID);
		if (renderer)
		{
			// use the already compiled shaders 
			video::CD3D9NormalMapRenderer* nmr = (video::CD3D9NormalMapRenderer*)renderer;
			VertexShader = nmr->VertexShader;
			if (VertexShader)
				VertexShader->AddRef();

			PixelShader = nmr->PixelShader;
			if (PixelShader)
				PixelShader->AddRef();

			outMaterialTypeNr = driver->addMaterialRenderer(this);
		}
		else
		{
			// compile shaders on our own
			if (driver->queryFeature(video::EVDF_PIXEL_SHADER_2_0))
			{
				init(outMaterialTypeNr, D3D9_NORMAL_MAP_VSH, D3D9_NORMAL_MAP_PSH_2_0);
			}
			else
			{
				init(outMaterialTypeNr, D3D9_NORMAL_MAP_VSH, D3D9_NORMAL_MAP_PSH_1_1);
			}
		}
		// something failed, use base material
		if (-1==outMaterialTypeNr)
			driver->addMaterialRenderer(this);
	}


	CD3D9NormalMapRenderer::~CD3D9NormalMapRenderer()
	{
		if (CallBack == this)
			CallBack = 0;
	}


	bool CD3D9NormalMapRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
	{
		if (vtxtype != video::EVT_TANGENTS)
		{
			os::Printer::log("Error: Normal map renderer only supports vertices of type EVT_TANGENTS", ELL_ERROR);
			return false;
		}

		return CD3D9ShaderMaterialRenderer::OnRender(service, vtxtype);
	}


	//! Returns the render capability of the material. 
	s32 CD3D9NormalMapRenderer::getRenderCapability() const
	{
		if (Driver->queryFeature(video::EVDF_PIXEL_SHADER_1_1) &&
			Driver->queryFeature(video::EVDF_VERTEX_SHADER_1_1))
			return 0;

		return 1;
	}


	//! Called by the engine when the vertex and/or pixel shader constants
	//! for an material renderer should be set.
	void CD3D9NormalMapRenderer::OnSetConstants(IMaterialRendererServices* services, s32 userData)
	{
		video::IVideoDriver* driver = services->getVideoDriver();

		// set transposed world matrix
		services->setVertexShaderConstant(driver->getTransform(video::ETS_WORLD).getTransposed().pointer(), 0, 4);

		// set transposed worldViewProj matrix
		core::matrix4 worldViewProj(driver->getTransform(video::ETS_PROJECTION));
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);
		services->setVertexShaderConstant(worldViewProj.getTransposed().pointer(), 8, 4);

		// here we've got to fetch the fixed function lights from the
		// driver and set them as constants

		u32 cnt = driver->getDynamicLightCount();

		for (u32 i=0; i<2; ++i)
		{
			SLight light; 

			if (i<cnt)
				light = driver->getDynamicLight(i);
			else
			{
				light.DiffuseColor.set(0,0,0); // make light dark
				light.Radius = 1.0f;
			}

			light.DiffuseColor.a = 1.0f/(light.Radius*light.Radius); // set attenuation

			services->setVertexShaderConstant(reinterpret_cast<const f32*>(&light.Position), 12+(i*2), 1);
			services->setVertexShaderConstant(reinterpret_cast<const f32*>(&light.DiffuseColor), 13+(i*2), 1);
		}

		// this is not really necessary in d3d9 (used a def instruction), but to be sure:
		f32 c95[] = {0.5f, 0.5f, 0.5f, 0.5f};
		services->setVertexShaderConstant(c95, 95, 1);
	}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_9_


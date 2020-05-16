// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_

#include "CD3D8NormalMapRenderer.h"
#include "IMaterialRendererServices.h"
#include "IVideoDriver.h"
#include "os.h"
#include "SLight.h"

namespace irr
{
namespace video  
{

	// 1.1 Shaders with two lights and vertex based attenuation

	// Irrlicht Engine D3D8 render path normal map vertex shader
	const char D3D8_NORMAL_MAP_VSH[] = 
		";Irrlicht Engine 0.8 D3D8 render path normal map vertex shader\n"\
		"; c0-3: Transposed world matrix \n"\
		"; c8-11: Transposed worldViewProj matrix (Projection * View * World) \n"\
		"; c12: Light01 position \n"\
		"; c13: x,y,z: Light01 color; .w: 1/LightRadius² \n"\
		"; c14: Light02 position \n"\
		"; c15: x,y,z: Light02 color; .w: 1/LightRadius² \n"\
		"\n"\
		"; v0 - position \n"\
		"; v1 - normal \n"\
		"; v2 - color \n"\
		"; v3 - texture coord \n"\
		"; v4 - tangent \n"\
		"; v5 - binormal \n"\
		"\n"\
		"vs.1.1\n"\
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

	// Irrlicht Engine D3D8 render path normal map pixel shader
	const char D3D8_NORMAL_MAP_PSH[] = 
		";Irrlicht Engine 0.8 D3D8 render path normal map pixel shader\n"\
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
		"mul r0, t0, r0             ; total luminance * base color\n"\
		"mov r0.a, v0.a             ; write interpolated vertex alpha value \n"\
		"\n"\
		"";	

	CD3D8NormalMapRenderer::CD3D8NormalMapRenderer(
		IDirect3DDevice8* d3ddev, video::IVideoDriver* driver, 
		s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial)
		: CD3D8ShaderMaterialRenderer(d3ddev, driver, 0, baseMaterial), 
		CompiledShaders(true)
	{

		#ifdef _DEBUG
		setDebugName("CD3D8NormalMapRenderer");
		#endif

		// set this as callback. We could have done this in 
		// the initialization list, but some compilers don't like it.

		CallBack = this;

		// basicly, this thing simply compiles these hardcoded shaders if the
		// hardware is able to do them, otherwise it maps to the base material

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
			video::CD3D8NormalMapRenderer* nmr = (video::CD3D8NormalMapRenderer*)renderer;
			CompiledShaders = false;

			VertexShader = nmr->VertexShader;
			PixelShader = nmr->PixelShader;

			outMaterialTypeNr = driver->addMaterialRenderer(this);
		}
		else
		{
			// compile shaders on our own
			init(outMaterialTypeNr, D3D8_NORMAL_MAP_VSH, D3D8_NORMAL_MAP_PSH, EVT_TANGENTS);
		}
		// something failed, use base material
		if (-1==outMaterialTypeNr)
			driver->addMaterialRenderer(this);
	}


	CD3D8NormalMapRenderer::~CD3D8NormalMapRenderer()
	{
		if (CallBack == this)
			CallBack = 0;

		if (!CompiledShaders)
		{
			// prevent this from deleting shaders we did not create 
			VertexShader = 0;
			PixelShader = 0;
		}
	}


	bool CD3D8NormalMapRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
	{
		if (vtxtype != video::EVT_TANGENTS)
		{
			os::Printer::log("Error: Normal map renderer only supports vertices of type EVT_TANGENTS", ELL_ERROR);
			return false;
		}

		return CD3D8ShaderMaterialRenderer::OnRender(service, vtxtype);
	}

	//! Returns the render capability of the material. 
	s32 CD3D8NormalMapRenderer::getRenderCapability() const
	{
		if (Driver->queryFeature(video::EVDF_PIXEL_SHADER_1_1) &&
			Driver->queryFeature(video::EVDF_VERTEX_SHADER_1_1))
			return 0;

		return 1;
	}


	//! Called by the engine when the vertex and/or pixel shader constants for an
	//! material renderer should be set.
	void CD3D8NormalMapRenderer::OnSetConstants(IMaterialRendererServices* services, s32 userData)
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

		f32 c95[] = {0.5f, 0.5f, 0.5f, 0.5f};
		services->setVertexShaderConstant(c95, 95, 1);
	}
	

} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_8_


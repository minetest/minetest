// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_OPENGL_

#include "COpenGLNormalMapRenderer.h"
#include "IGPUProgrammingServices.h"
#include "IShaderConstantSetCallBack.h"
#include "IVideoDriver.h"
#include "os.h"
#include "COpenGLDriver.h"

namespace irr
{
namespace video
{

// Irrlicht Engine OpenGL render path normal map vertex shader
// I guess it could be optimized a lot, because I wrote it in D3D ASM and
// transferred it 1:1 to OpenGL
const char OPENGL_NORMAL_MAP_VSH[] =
	"!!ARBvp1.0\n"\
	"#input\n"\
	"# 0-3: transposed world matrix;\n"\
	"#;12: Light01 position \n"\
	"#;13: x,y,z: Light01 color; .w: 1/LightRadius^2 \n"\
	"#;14: Light02 position \n"\
	"#;15: x,y,z: Light02 color; .w: 1/LightRadius^2 \n"\
	"\n"\
	"ATTRIB InPos = vertex.position;\n"\
	"ATTRIB InColor = vertex.color;\n"\
	"ATTRIB InNormal = vertex.normal;\n"\
	"ATTRIB InTexCoord = vertex.texcoord[0];\n"\
	"ATTRIB InTangent = vertex.texcoord[1];\n"\
	"ATTRIB InBinormal = vertex.texcoord[2];\n"\
	"\n"\
	"#output\n"\
	"OUTPUT OutPos = result.position;\n"\
	"OUTPUT OutLightColor1 = result.color.primary;\n"\
	"OUTPUT OutLightColor2 = result.color.secondary;\n"\
	"OUTPUT OutTexCoord = result.texcoord[0];\n"\
	"OUTPUT OutLightVector1 = result.texcoord[1];\n"\
	"OUTPUT OutLightVector2 = result.texcoord[2];\n"\
	"\n"\
	"PARAM MVP[4] = { state.matrix.mvp }; # modelViewProjection matrix.\n"\
	"TEMP Temp;\n"\
	"TEMP TempColor;\n"\
	"TEMP TempLightVector1;\n"\
	"TEMP TempLightVector2;\n"\
	"TEMP TempTransLightV1;\n"\
	"TEMP TempTransLightV2;\n"\
	"\n"\
	"# transform position to clip space \n"\
	"DP4 OutPos.x, MVP[0], InPos;\n"\
	"DP4 OutPos.y, MVP[1], InPos;\n"\
	"DP4 Temp.z, MVP[2], InPos;\n"\
	"DP4 OutPos.w, MVP[3], InPos;\n"\
	"MOV OutPos.z, Temp.z;\n"\
	"MOV result.fogcoord.x, Temp.z;\n"\
	"\n"\
	"# vertex - lightpositions \n"\
	"SUB TempLightVector1, program.local[12], InPos; \n"\
	"SUB TempLightVector2, program.local[14], InPos; \n"\
	"\n"\
	"# transform the light vector 1 with U, V, W \n"\
	"DP3 TempTransLightV1.x, InTangent, TempLightVector1; \n"\
	"DP3 TempTransLightV1.y, InBinormal, TempLightVector1; \n"\
	"DP3 TempTransLightV1.z, InNormal, TempLightVector1; \n"\
	"\n"\
	"# transform the light vector 2 with U, V, W \n"\
	"DP3 TempTransLightV2.x, InTangent, TempLightVector2; \n"\
	"DP3 TempTransLightV2.y, InBinormal, TempLightVector2; \n"\
	"DP3 TempTransLightV2.z, InNormal, TempLightVector2; \n"\
	"\n"\
	"# normalize light vector 1 \n"\
	"DP3 TempTransLightV1.w, TempTransLightV1, TempTransLightV1; \n"\
	"RSQ TempTransLightV1.w, TempTransLightV1.w; \n"\
	"MUL TempTransLightV1, TempTransLightV1, TempTransLightV1.w;\n"\
	"\n"\
	"# normalize light vector 2 \n"\
	"DP3 TempTransLightV2.w, TempTransLightV2, TempTransLightV2; \n"\
	"RSQ TempTransLightV2.w, TempTransLightV2.w; \n"\
	"MUL TempTransLightV2, TempTransLightV2, TempTransLightV2.w;\n"\
	"\n"\
	"\n"\
	"# move light vectors out\n"\
	"MAD OutLightVector1, TempTransLightV1, {0.5,0.5,0.5,0.5}, {0.5,0.5,0.5,0.5}; \n"\
	"MAD OutLightVector2, TempTransLightV2, {0.5,0.5,0.5,0.5}, {0.5,0.5,0.5,0.5}; \n"\
	"\n"\
	"# calculate attenuation of light 1\n"\
	"MOV TempLightVector1.w, {0,0,0,0}; \n"\
	"DP3 TempLightVector1.x, TempLightVector1, TempLightVector1; \n"\
	"MUL TempLightVector1.x, TempLightVector1.x, program.local[13].w;  \n"\
	"RSQ TempLightVector1, TempLightVector1.x; \n"\
	"MUL OutLightColor1, TempLightVector1, program.local[13]; # resulting light color = lightcolor * attenuation \n"\
	"\n"\
	"# calculate attenuation of light 2\n"\
	"MOV TempLightVector2.w, {0,0,0,0}; \n"\
	"DP3 TempLightVector2.x, TempLightVector2, TempLightVector2; \n"\
	"MUL TempLightVector2.x, TempLightVector2.x, program.local[15].w;  \n"\
	"RSQ TempLightVector2, TempLightVector2.x; \n"\
	"MUL OutLightColor2, TempLightVector2, program.local[15]; # resulting light color = lightcolor * attenuation \n"\
	"\n"\
	"# move out texture coordinates and original alpha value\n"\
	"MOV OutTexCoord, InTexCoord; \n"\
	"MOV OutLightColor1.w, InColor.w; \n"\
	"\n"\
	"END\n";

// Irrlicht Engine OpenGL render path normal map pixel shader
// I guess it could be optimized a bit, because I wrote it in D3D ASM and
// transfered it 1:1 to OpenGL
const char OPENGL_NORMAL_MAP_PSH[] =
	"!!ARBfp1.0\n"\
	"#_IRR_FOG_MODE_\n"\
	"\n"\
	"#Input\n"\
	"ATTRIB inTexCoord = fragment.texcoord[0];   \n"\
	"ATTRIB light1Vector = fragment.texcoord[1]; \n"\
	"ATTRIB light2Vector = fragment.texcoord[2];    \n"\
	"ATTRIB light1Color = fragment.color.primary;   \n"\
	"ATTRIB light2Color = fragment.color.secondary; \n"\
	"\n"\
	"#Output\n"\
	"OUTPUT outColor = result.color;\n"\
	"TEMP temp;\n"\
	"TEMP temp2;\n"\
	"TEMP colorMapColor;\n"\
	"TEMP normalMapColor;\n"\
	"\n"\
	"# fetch color and normal map; \n"\
	"TXP colorMapColor, inTexCoord, texture[0], 2D; \n"\
	"TXP normalMapColor, inTexCoord, texture[1], 2D; \n"\
	"\n"\
	"# calculate color of light1; \n"\
	"MAD normalMapColor, normalMapColor, {2,2,2,2}, {-1,-1,-1,-1}; \n"\
	"MAD temp, light1Vector, {2,2,2,2}, {-1,-1,-1,-1}; \n"\
	"DP3_SAT temp, normalMapColor, temp; \n"\
	"MUL temp, light1Color, temp; \n"\
	"\n"\
	"# calculate color of light2; \n"\
	"MAD temp2, light2Vector, {2,2,2,2}, {-1,-1,-1,-1}; \n"\
	"DP3_SAT temp2, normalMapColor, temp2; \n"\
	"MAD temp, light2Color, temp2, temp; \n"\
	"\n"\
	"# luminance * base color; \n"\
	"MUL outColor, temp, colorMapColor; \n"\
	"MOV outColor.a, light1Color.a; #write interpolated vertex alpha value\n"\
	"\n"\
	"END\n";

//! Constructor
COpenGLNormalMapRenderer::COpenGLNormalMapRenderer(video::COpenGLDriver* driver,
	s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial)
	: COpenGLShaderMaterialRenderer(driver, 0, baseMaterial), CompiledShaders(true)
{

	#ifdef _DEBUG
	setDebugName("COpenGLNormalMapRenderer");
	#endif

	// set this as callback. We could have done this in
	// the initialization list, but some compilers don't like it.

	CallBack = this;

	// basically, this thing simply compiles the hardcoded shaders if the
	// hardware is able to do them, otherwise it maps to the base material

	if (!driver->queryFeature(video::EVDF_ARB_FRAGMENT_PROGRAM_1) ||
		!driver->queryFeature(video::EVDF_ARB_VERTEX_PROGRAM_1))
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
		video::COpenGLNormalMapRenderer* nmr = reinterpret_cast<video::COpenGLNormalMapRenderer*>(renderer);
		CompiledShaders = false;

		VertexShader = nmr->VertexShader;
		PixelShader = nmr->PixelShader;

		outMaterialTypeNr = driver->addMaterialRenderer(this);
	}
	else
	{
		// compile shaders on our own
		init(outMaterialTypeNr, OPENGL_NORMAL_MAP_VSH, OPENGL_NORMAL_MAP_PSH, EVT_TANGENTS);
	}

	// fallback if compilation has failed
	if (-1==outMaterialTypeNr)
		outMaterialTypeNr = driver->addMaterialRenderer(this);
}


//! Destructor
COpenGLNormalMapRenderer::~COpenGLNormalMapRenderer()
{
	if (CallBack == this)
		CallBack = 0;

	if (!CompiledShaders)
	{
		// prevent this from deleting shaders we did not create
		VertexShader = 0;
		PixelShader.clear();
	}
}


//! Returns the render capability of the material.
s32 COpenGLNormalMapRenderer::getRenderCapability() const
{
	if (Driver->queryFeature(video::EVDF_ARB_FRAGMENT_PROGRAM_1) &&
		Driver->queryFeature(video::EVDF_ARB_VERTEX_PROGRAM_1))
		return 0;

	return 1;
}


//! Called by the engine when the vertex and/or pixel shader constants for an
//! material renderer should be set.
void COpenGLNormalMapRenderer::OnSetConstants(IMaterialRendererServices* services, s32 userData)
{
	video::IVideoDriver* driver = services->getVideoDriver();

	// set transposed world matrix
	const core::matrix4& tWorld = driver->getTransform(video::ETS_WORLD).getTransposed();
	services->setVertexShaderConstant(tWorld.pointer(), 0, 4);

	// set transposed worldViewProj matrix
	core::matrix4 worldViewProj(driver->getTransform(video::ETS_PROJECTION));
	worldViewProj *= driver->getTransform(video::ETS_VIEW);
	worldViewProj *= driver->getTransform(video::ETS_WORLD);
	core::matrix4 tr(worldViewProj.getTransposed());
	services->setVertexShaderConstant(tr.pointer(), 8, 4);

	// here we fetch the fixed function lights from the driver
	// and set them as constants

	u32 cnt = driver->getDynamicLightCount();

	// Load the inverse world matrix.
	core::matrix4 invWorldMat;
	driver->getTransform(video::ETS_WORLD).getInverse(invWorldMat);

	for (u32 i=0; i<2; ++i)
	{
		video::SLight light;

		if (i<cnt)
			light = driver->getDynamicLight(i);
		else
		{
			light.DiffuseColor.set(0,0,0); // make light dark
			light.Radius = 1.0f;
		}

		light.DiffuseColor.a = 1.0f/(light.Radius*light.Radius); // set attenuation

		// Transform the light by the inverse world matrix to get it into object space.
		invWorldMat.transformVect(light.Position);

		services->setVertexShaderConstant(
			reinterpret_cast<const f32*>(&light.Position), 12+(i*2), 1);

		services->setVertexShaderConstant(
			reinterpret_cast<const f32*>(&light.DiffuseColor), 13+(i*2), 1);
	}
}


} // end namespace video
} // end namespace irr


#endif


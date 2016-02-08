#include "IShaderDefaultPostProcessCallback.h"
#include "debug.h"
#include "camera.h"

irr::f32 random_f(irr::f32 min, irr::f32 max)
{
	return rand()/(irr::f32)RAND_MAX*(max-min)+min;
}

irr::video::IShaderDefaultPostProcessCallback::IShaderDefaultPostProcessCallback()
{
}

void irr::video::IShaderDefaultPostProcessCallback::OnSetConstants(
		irr::video::IMaterialRendererServices* services,
		irr::s32 userData)
{
    video::IVideoDriver *driver = services->getVideoDriver();
	sanity_check(driver);

	// set inverted world matrix
	core::matrix4 invWorld = driver->getTransform(video::ETS_WORLD);
	invWorld.makeInverse();
	services->setPixelShaderConstant("mInvWorld", invWorld.pointer(), 16);
	
	// set clip matrix
	core::matrix4 worldViewProj;
	worldViewProj = driver->getTransform(video::ETS_PROJECTION);
	services->setPixelShaderConstant("mProjectionMatrix", worldViewProj.pointer(), 16);
		
	core::matrix4 invWorldViewProj;
	invWorldViewProj = driver->getTransform(video::ETS_PROJECTION);
	invWorldViewProj.makeInverse();
	services->setPixelShaderConstant("mInverseProjectionMatrix", worldViewProj.pointer(), 16);
		
	core::matrix4 transWorld = driver->getTransform(video::ETS_WORLD);
	transWorld = transWorld.getTransposed();
	services->setPixelShaderConstant("mTransWorld", transWorld.pointer(), 16);

	// set world matrix
	core::matrix4 world = driver->getTransform(video::ETS_WORLD);
	services->setPixelShaderConstant("mWorld", world.pointer(), 16);

	for (int i = 0; i < 32; ++i) {
		ssaoKernel[i] = irr::core::vector3df(
			random_f(-1.0f, 1.0f),
			random_f(-1.0f, 1.0f),
			random_f(0.0f, 1.0f));
	}
	
	irr::s32 render = 0;
	irr::s32 tex0 = 1;
	irr::s32 tex1 = 2;
	irr::s32 tex2 = 3;

	services->setPixelShaderConstant("Render", &render, 1);
	services->setPixelShaderConstant("Tex0", &tex0, 1);   
	services->setPixelShaderConstant("Tex1", &tex1, 1);   
	services->setPixelShaderConstant("Tex2", &tex2, 1);   

	services->setPixelShaderConstant("PixelSize", (irr::f32*)&PixelSize, 2);

	services->setPixelShaderConstant("width", (irr::f32*)&ScreenWidth, 1);
	services->setPixelShaderConstant("height", (irr::f32*)&ScreenHeight, 1);

	services->setPixelShaderConstant("ssaoKernel", (irr::f32*)&ssaoKernel, 32);
}

void irr::video::IShaderDefaultPostProcessCallback::OnSetMaterial (
		const SMaterial &material)
{
    irr::core::dimension2du tSize = material.getTexture(0)->getSize();
    PixelSize = irr::core::vector2df(1.0 / tSize.Width, 1.0 / tSize.Height);
	ScreenWidth = tSize.Width;
	ScreenHeight = tSize.Height;

	NumTextures = 0;
    for (u32 i = 1; i <= 4; i++)
    {
        if (material.getTexture(i))
            NumTextures++;
        else
            break;
    }
}

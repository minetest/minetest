#include "IShaderDefaultPostProcessCallback.h"
#include "debug.h"

irr::video::IShaderDefaultPostProcessCallback::IShaderDefaultPostProcessCallback()
{
}

void irr::video::IShaderDefaultPostProcessCallback::OnSetConstants(
		irr::video::IMaterialRendererServices* services,
		irr::s32 userData)
{
    irr::s32 render = 0;
	irr::s32 tex0 = 1;
    irr::s32 tex1 = 2;
    irr::s32 tex2 = 3;

    services->setPixelShaderConstant("Render", &render, 1);
	services->setPixelShaderConstant("Tex0", &tex0, 1);   
    services->setPixelShaderConstant("Tex1", &tex1, 1);   
	services->setPixelShaderConstant("Tex2", &tex2, 1);   

    services->setPixelShaderConstant("PixelSize", (irr::f32*)&PixelSize, 2);
}

void irr::video::IShaderDefaultPostProcessCallback::OnSetMaterial (
		const SMaterial &material)
{
    irr::core::dimension2du tSize = material.getTexture(0)->getSize();
    PixelSize = irr::core::vector2df(1.0 / tSize.Width, 1.0 / tSize.Height);
	NumTextures = 0;
    for (u32 i = 1; i <= 4; i++)
    {
        if (material.getTexture(i))
            NumTextures++;
        else
            break;
    }
}

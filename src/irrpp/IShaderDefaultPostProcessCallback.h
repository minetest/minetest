#ifndef ISHADERDEFAULTPOSTPROCESSCALLBACK_H
#define ISHADERDEFAULTPOSTPROCESSCALLBACK_H

#include <irrlicht.h>

namespace irr
{
namespace video
{

class IShaderDefaultPostProcessCallback : public irr::video::IShaderConstantSetCallBack
{
    public:
        IShaderDefaultPostProcessCallback();

        virtual void OnSetConstants(irr::video::IMaterialRendererServices* services, irr::s32 userData);

        virtual void OnSetMaterial (const SMaterial &material);

    private:
        irr::u32 NumTextures;
        irr::core::vector2df PixelSize;
		irr::f32 ScreenWidth;
		irr::f32 ScreenHeight;
        irr::u32 RenderID;
        irr::u32 TexIDs[3];
        irr::u32 PixelSizeID;
};

}
}

#endif // ISHADERDEFAULTPOSTPROCESSCALLBACK_H

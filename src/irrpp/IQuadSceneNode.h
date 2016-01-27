// This file is part of the "irrRenderer".
// For conditions of distribution and use, see copyright notice in irrRenderer.h

#ifndef IQUADSCENENODE_H
#define IQUADSCENENODE_H

#include <irrlicht.h>

namespace irr{
namespace scene{

class IQuadSceneNode : public irr::scene::ISceneNode
{
    public:
        IQuadSceneNode(irr::scene::ISceneNode* parent, irr::scene::ISceneManager* mgr, irr::s32 id=-1);
        virtual ~IQuadSceneNode();
        void render();
        void OnRegisterSceneNode();
        const irr::core::aabbox3d<irr::f32>& getBoundingBox() const;
        void setMaterialTexture(irr::u32 layer, irr::video::ITexture* texture);
        void setMaterialType(irr::video::E_MATERIAL_TYPE tehType);
        irr::video::SMaterial& getMaterial(irr::u32 num);
        irr::u32 getMaterialCount() const;

    protected:
        irr::video::SMaterial Material;
        irr::scene::SMeshBuffer* Buffer;
};

}
}

#endif // IQUADSCENENODE_H

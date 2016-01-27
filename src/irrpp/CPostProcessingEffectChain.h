// This file is part of the "irrRenderer".
// For conditions of distribution and use, see copyright notice in irrRenderer.h

#ifndef CPOSTPROCESSINGEFFECTCHAIN_H
#define CPOSTPROCESSINGEFFECTCHAIN_H

#include <irrlicht.h>
#include "CPostProcessingEffect.h"
#include "E_POST_PROCESSING_EFFECT.h"
#include "IShaderDefaultPostProcessCallback.h"

namespace irr
{
namespace video
{

//class irrPP;

//! A chain that can hold post-processing effects and render them in a set order.
/**
 * Create one using irr::video::CRenderer.
 */
class CPostProcessingEffectChain
{
    public:
        /**
         * Constructor, only used internally.
         * @param renderer renderer this chain belongs to
         */
        CPostProcessingEffectChain(irr::IrrlichtDevice* device, irr::video::E_POSTPROCESSING_EFFECT_QUALITY defQuality, irr::io::path shaderDir);
        /**
         * Destructor.
         */
        ~CPostProcessingEffectChain();

        /**
         * Attaches a new effect to this chain.
         * @param effect pointer to an effect to attach
         * @return Internal index in the chain of the newly attached effect.
         */
        irr::u32 attachEffect(irr::video::CPostProcessingEffect* effect);
        /**
         * Creates a new effect and attaches it to this chain.
         * @param effectShader shader to use for the new effect
         * @param callback callback for the shader, 0 for none
         * @return Pointer to the newly created effect.
         */
        irr::video::CPostProcessingEffect* createEffect(irr::core::stringc sourceFrag, irr::video::IShaderConstantSetCallBack* callback= new irr::video::IShaderDefaultPostProcessCallback);
        /**
         * Creates a new effect and attaches it to this chain.
         * @param type one of the included post-processing effect
         * @return The newly created effect.
         */
        irr::video::CPostProcessingEffect* createEffect(irr::video::E_POSTPROCESSING_EFFECT type);

        /**
         * Detaches the effect on the given index from the chain.
         * @param index the index of the effect in the chain
         */
        void detachEffect(irr::u32 index);
        /**
         * Detaches the effect pointed to by the pointer.
         * @param effect pointer to the effect whi
         ch should be detached
         */
        void detachEffect(irr::video::CPostProcessingEffect* effect);

        /**
         * Removes the effect on the given index.
         * @param index the index of the effect in the chain
         */
        void removeEffect(irr::u32 index);
        /**
         * Removes the effect pointed to by the pointer.
         * @param effect pointer to the effect which should be detached
         */
        void removeEffect(irr::video::CPostProcessingEffect* effect);
        /**
         * Sets whether this chain should be active.
         * @param active whether the chain should be active or not
         */
        void setActive(bool active);
        /**
         * Returns whether is this chain active
         * @return True if active
         */
        bool isActive();

        /**
         * Set to true in order for the chain to preserve it's original input render. The render will be available from irr::video::CPostProcessingEffect::getOriginalRender.
         * @param keep true to keep, false frees the render texture if there was one created with true before
         */
        void setKeepOriginalRender(bool keep);
        /**
         * Returns whether the original render is being kept inside this chain.
         * @return True if yes.
         */
        bool getKeepOriginalRender() const;
        /**
         * Returns the original render texture if irr::video::CPostProcessingEffect::setKeepOriginalTexture was called with true before.
         * @return The original render texture.
         */
        irr::video::ITexture* getOriginalRender();

        /**
         * Requests that the amount of active effects be recalculated. Only used internally.
         */
        void requestActiveEffectUpdate();
        /**
         * Gets how many effects there are in this chain
         * @return The amount of effects belonging to this chain
         */
        irr::u32 getEffectCount() const;
        /**
         * Gets how many active effects there are in this chain
         * @return The amount of active effects belonging to this chain
         */
        irr::u32 getActiveEffectCount() const;
        /**
         * Gets pointer to the effect on the given index in the chain.
         * @param index index of the effect, starting from 0
         * @return Pointer to the effect on the given index.
         */
        irr::video::CPostProcessingEffect* getEffectFromIndex(irr::u32 index);
        /**
         * Returns the internal index of an effect pointed to by the pointer attached to this chain.
         * @param effect effect to return index for
         * @return The index of the effect.
         */
        irr::u32 getEffectIndex(irr::video::CPostProcessingEffect* effect) const;

        irr::core::stringc readShader(irr::io::path filename) const;

    private:

        irr::IrrlichtDevice* Device;
        irr::video::E_POSTPROCESSING_EFFECT_QUALITY DefaultQuality;
        irr::io::path ShaderDir;

        bool Active;
        irr::u32 ActiveEffectCount;
        irr::video::ITexture* OriginalRender;

        irr::core::array<irr::video::CPostProcessingEffect*> Effects;
};

}
}

#endif // CPOSTPROCESSINGEFFECTCHAIN_H

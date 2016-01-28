// This file is part of the "irrRenderer".
// For conditions of distribution and use, see copyright notice in irrRenderer.h

#ifndef E_POSTPROCESSING_EFFECT_H
#define E_POSTPROCESSING_EFFECT_H

namespace irr
{
namespace video
{

//! Enumerators for the included post-processing effects.
enum E_POSTPROCESSING_EFFECT
{
    //! You see red.
    EPE_I_SEE_RED = 0,
    //! Does nothing.
    EPE_ALBEDO,
    //! Adds the input render to another texture
    EPE_ADD2,

    //! FXAA antialiasing
    EPE_FXAA,

    //! Only let bright areas of the render pass through
    EPE_BLOOM_PREPASS,

    //! Blur render vertically, taking a sample of 5 pixels
    EPE_BLUR_V_LOW,
    //! Blur render horizontally, taking a sample of 5 pixels
    EPE_BLUR_H_LOW,
    //! Blur horizontally and add to a supplied texture (on index 0), taking a sample of 5 pixels
    EPE_BLUR_H_ADD_LOW,

    //! Blur render vertically, taking a sample of 13 pixels
    EPE_BLUR_V_MEDIUM,
    //! Blur render horizontally, taking a sample of 13 pixels
    EPE_BLUR_H_MEDIUM,
    //! Blur horizontally and add to a supplied texture (on index 0), taking a sample of 13 pixels
    EPE_BLUR_H_ADD_MEDIUM,

    //! Blur render vertically, taking a sample of (practically) 25 pixels (21, weights adjusted for 25)
    EPE_BLUR_V_HIGH,
    //! Blur render horizontally, taking a sample of (practically) 25 pixels (21, weights adjusted for 25)
    EPE_BLUR_H_HIGH,
    //! Blur horizontally and add to a supplied texture (on index 0), taking a sample of (practically) 25 pixels (21, weights adjusted for 25)
    EPE_BLUR_H_ADD_HIGH,

    //! Pixel fog
    EPE_FOG,
    EPE_COLD_COLORS,
    EPE_WARM_COLORS,
};

}
}

#endif

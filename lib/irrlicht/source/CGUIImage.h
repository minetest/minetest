// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_GUI_IMAGE_H_INCLUDED__
#define __C_GUI_IMAGE_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUIImage.h"

namespace irr
{
namespace gui
{

	class CGUIImage : public IGUIImage
	{
	public:

		//! constructor
		CGUIImage(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle);

		//! destructor
		virtual ~CGUIImage();

		//! sets an image
		virtual void setImage(video::ITexture* image);

		//! Gets the image texture
		virtual video::ITexture* getImage() const;

		//! sets the color of the image
		virtual void setColor(video::SColor color);

		//! sets if the image should scale to fit the element
		virtual void setScaleImage(bool scale);

		//! draws the element and its children
		virtual void draw();

		//! sets if the image should use its alpha channel to draw itself
		virtual void setUseAlphaChannel(bool use);

		//! Gets the color of the image
		virtual video::SColor getColor() const;

		//! Returns true if the image is scaled to fit, false if not
		virtual bool isImageScaled() const;

		//! Returns true if the image is using the alpha channel, false if not
		virtual bool isAlphaChannelUsed() const;

		//! Writes attributes of the element.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

		//! Reads attributes of the element
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

	private:
		video::ITexture* Texture;
		video::SColor Color;
		bool UseAlphaChannel;
		bool ScaleImage;

	};


} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

#endif // __C_GUI_IMAGE_H_INCLUDED__

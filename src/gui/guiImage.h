// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "irrlichttypes_extrabloated.h"
#include <irrlicht/IGUIEnvironment.h>
#include <irrlicht/IGUIImage.h>

class GUIImage : public gui::IGUIImage
{
public:

	//! constructor
	GUIImage(gui::IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle);

	//! destructor
	virtual ~GUIImage();

	//! sets an image
	void setImage(video::ITexture* image) override;

	//! Gets the image texture
	video::ITexture* getImage() const override;

	//! sets the color of the image
	void setColor(video::SColor color) override;

	//! sets if the image should scale to fit the element
	void setScaleImage(bool scale) override;

	//! draws the element and its children
	void draw() override;

	//! sets if the image should use its alpha channel to draw itself
	void setUseAlphaChannel(bool use) override;

	//! Gets the color of the image
	video::SColor getColor() const override;

	//! Returns true if the image is scaled to fit, false if not
	bool isImageScaled() const override;

	//! Returns true if the image is using the alpha channel, false if not
	bool isAlphaChannelUsed() const override;

	//! Sets the source rectangle of the image. By default the full image is used.
	void setSourceRect(const core::rect<s32>& sourceRect);

	//! Returns the customized source rectangle of the image to be used.
	core::rect<s32> getSourceRect() const;

	//! Restrict drawing-area.
	void setDrawBounds(const core::rect<f32>& drawBoundUVs);

	//! Get drawing-area restrictions.
	core::rect<f32> getDrawBounds() const;

	//! Sets whether to draw a background color (EGDC_3D_DARK_SHADOW) when no texture is set
	void setDrawBackground(bool draw)
	{
		DrawBackground = draw;
	}

	//! Checks if a background is drawn when no texture is set
	bool isDrawBackgroundEnabled() const
	{
		return DrawBackground;
	}

	//! Writes attributes of the element.
	void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const override;

	//! Reads attributes of the element
	void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options) override;

protected:
	void checkBounds(core::rect<s32>& rect)
	{
		f32 clipWidth = (f32)rect.getWidth();
		f32 clipHeight = (f32)rect.getHeight();

		rect.UpperLeftCorner.X += core::round32(DrawBounds.UpperLeftCorner.X*clipWidth);
		rect.UpperLeftCorner.Y += core::round32(DrawBounds.UpperLeftCorner.Y*clipHeight);
		rect.LowerRightCorner.X -= core::round32((1.f-DrawBounds.LowerRightCorner.X)*clipWidth);
		rect.LowerRightCorner.Y -= core::round32((1.f-DrawBounds.LowerRightCorner.Y)*clipHeight);
	}

private:
	video::ITexture* Texture;
	video::SColor Color;
	bool UseAlphaChannel;
	bool ScaleImage;
	core::rect<s32> SourceRect;
	core::rect<f32> DrawBounds;
	bool DrawBackground;
};


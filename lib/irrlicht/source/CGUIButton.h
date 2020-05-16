// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_GUI_BUTTON_H_INCLUDED__
#define __C_GUI_BUTTON_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUIButton.h"
#include "IGUISpriteBank.h"
#include "SColor.h"

namespace irr
{
namespace gui
{

	class CGUIButton : public IGUIButton
	{
	public:

		//! constructor
		CGUIButton(IGUIEnvironment* environment, IGUIElement* parent,
			s32 id, core::rect<s32> rectangle, bool noclip=false);

		//! destructor
		virtual ~CGUIButton();

		//! called if an event happened.
		virtual bool OnEvent(const SEvent& event);

		//! draws the element and its children
		virtual void draw();

		//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
		virtual void setOverrideFont(IGUIFont* font=0);

		//! Gets the override font (if any)
		virtual IGUIFont* getOverrideFont() const;

		//! Get the font which is used right now for drawing
		virtual IGUIFont* getActiveFont() const;

		//! Sets an image which should be displayed on the button when it is in normal state.
		virtual void setImage(video::ITexture* image=0);

		//! Sets an image which should be displayed on the button when it is in normal state.
		virtual void setImage(video::ITexture* image, const core::rect<s32>& pos);

		//! Sets an image which should be displayed on the button when it is in pressed state.
		virtual void setPressedImage(video::ITexture* image=0);

		//! Sets an image which should be displayed on the button when it is in pressed state.
		virtual void setPressedImage(video::ITexture* image, const core::rect<s32>& pos);

		//! Sets the sprite bank used by the button
		virtual void setSpriteBank(IGUISpriteBank* bank=0);

		//! Sets the animated sprite for a specific button state
		/** \param index: Number of the sprite within the sprite bank, use -1 for no sprite
		\param state: State of the button to set the sprite for
		\param index: The sprite number from the current sprite bank
		\param color: The color of the sprite
		*/
		virtual void setSprite(EGUI_BUTTON_STATE state, s32 index,
				video::SColor color=video::SColor(255,255,255,255), bool loop=false);

		//! Sets if the button should behave like a push button. Which means it
		//! can be in two states: Normal or Pressed. With a click on the button,
		//! the user can change the state of the button.
		virtual void setIsPushButton(bool isPushButton=true);

		//! Checks whether the button is a push button
		virtual bool isPushButton() const;

		//! Sets the pressed state of the button if this is a pushbutton
		virtual void setPressed(bool pressed=true);

		//! Returns if the button is currently pressed
		virtual bool isPressed() const;

		//! Sets if the button should use the skin to draw its border
		virtual void setDrawBorder(bool border=true);

		//! Checks if the button face and border are being drawn
		virtual bool isDrawingBorder() const;

		//! Sets if the alpha channel should be used for drawing images on the button (default is false)
		virtual void setUseAlphaChannel(bool useAlphaChannel=true);

		//! Checks if the alpha channel should be used for drawing images on the button
		virtual bool isAlphaChannelUsed() const;

		//! Sets if the button should scale the button images to fit
		virtual void setScaleImage(bool scaleImage=true);

		//! Checks whether the button scales the used images
		virtual bool isScalingImage() const;

		//! Writes attributes of the element.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

		//! Reads attributes of the element
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

	private:

		struct ButtonSprite
		{
			s32 Index;
			video::SColor Color;
			bool Loop;
		};

		ButtonSprite ButtonSprites[EGBS_COUNT];

		IGUISpriteBank* SpriteBank;
		IGUIFont* OverrideFont;

		video::ITexture* Image;
		video::ITexture* PressedImage;

		core::rect<s32> ImageRect;
		core::rect<s32> PressedImageRect;

		u32 ClickTime, HoverTime, FocusTime;

		bool IsPushButton;
		bool Pressed;
		bool UseAlphaChannel;
		bool DrawBorder;
		bool ScaleImage;
	};

} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

#endif // __C_GUI_BUTTON_H_INCLUDED__


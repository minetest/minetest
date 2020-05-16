// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_BUTTON_H_INCLUDED__
#define __I_GUI_BUTTON_H_INCLUDED__

#include "IGUIElement.h"

namespace irr
{

namespace video
{
	class ITexture;
} // end namespace video

namespace gui
{
	class IGUIFont;
	class IGUISpriteBank;

	enum EGUI_BUTTON_STATE
	{
		//! The button is not pressed
		EGBS_BUTTON_UP=0,
		//! The button is currently pressed down
		EGBS_BUTTON_DOWN,
		//! The mouse cursor is over the button
		EGBS_BUTTON_MOUSE_OVER,
		//! The mouse cursor is not over the button
		EGBS_BUTTON_MOUSE_OFF,
		//! The button has the focus
		EGBS_BUTTON_FOCUSED,
		//! The button doesn't have the focus
		EGBS_BUTTON_NOT_FOCUSED,
		//! not used, counts the number of enumerated items
		EGBS_COUNT
	};

	//! Names for gui button state icons
	const c8* const GUIButtonStateNames[] =
	{
		"buttonUp",
		"buttonDown",
		"buttonMouseOver",
		"buttonMouseOff",
		"buttonFocused",
		"buttonNotFocused",
		0,
		0,
	};

	//! GUI Button interface.
	/** \par This element can create the following events of type EGUI_EVENT_TYPE:
	\li EGET_BUTTON_CLICKED
	*/
	class IGUIButton : public IGUIElement
	{
	public:

		//! constructor
		IGUIButton(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_BUTTON, environment, parent, id, rectangle) {}

		//! Sets another skin independent font.
		/** If this is set to zero, the button uses the font of the skin.
		\param font: New font to set. */
		virtual void setOverrideFont(IGUIFont* font=0) = 0;

		//! Gets the override font (if any)
		/** \return The override font (may be 0) */
		virtual IGUIFont* getOverrideFont(void) const = 0;

		//! Get the font which is used right now for drawing
		/** Currently this is the override font when one is set and the
		font of the active skin otherwise */
		virtual IGUIFont* getActiveFont() const = 0;

		//! Sets an image which should be displayed on the button when it is in normal state.
		/** \param image: Image to be displayed */
		virtual void setImage(video::ITexture* image=0) = 0;

		//! Sets a background image for the button when it is in normal state.
		/** \param image: Texture containing the image to be displayed
		\param pos: Position in the texture, where the image is located */
		virtual void setImage(video::ITexture* image, const core::rect<s32>& pos) = 0;

		//! Sets a background image for the button when it is in pressed state.
		/** If no images is specified for the pressed state via
		setPressedImage(), this image is also drawn in pressed state.
		\param image: Image to be displayed */
		virtual void setPressedImage(video::ITexture* image=0) = 0;

		//! Sets an image which should be displayed on the button when it is in pressed state.
		/** \param image: Texture containing the image to be displayed
		\param pos: Position in the texture, where the image is located */
		virtual void setPressedImage(video::ITexture* image, const core::rect<s32>& pos) = 0;

		//! Sets the sprite bank used by the button
		virtual void setSpriteBank(IGUISpriteBank* bank=0) = 0;

		//! Sets the animated sprite for a specific button state
		/** \param index: Number of the sprite within the sprite bank, use -1 for no sprite
		\param state: State of the button to set the sprite for
		\param index: The sprite number from the current sprite bank
		\param color: The color of the sprite
		\param loop: True if the animation should loop, false if not
		*/
		virtual void setSprite(EGUI_BUTTON_STATE state, s32 index,
				video::SColor color=video::SColor(255,255,255,255), bool loop=false) = 0;

		//! Sets if the button should behave like a push button.
		/** Which means it can be in two states: Normal or Pressed. With a click on the button,
		the user can change the state of the button. */
		virtual void setIsPushButton(bool isPushButton=true) = 0;

		//! Sets the pressed state of the button if this is a pushbutton
		virtual void setPressed(bool pressed=true) = 0;

		//! Returns if the button is currently pressed
		virtual bool isPressed() const = 0;

		//! Sets if the alpha channel should be used for drawing background images on the button (default is false)
		virtual void setUseAlphaChannel(bool useAlphaChannel=true) = 0;

		//! Returns if the alpha channel should be used for drawing background images on the button
		virtual bool isAlphaChannelUsed() const = 0;

		//! Returns whether the button is a push button
		virtual bool isPushButton() const = 0;

		//! Sets if the button should use the skin to draw its border and button face (default is true)
		virtual void setDrawBorder(bool border=true) = 0;

		//! Returns if the border and button face are being drawn using the skin
		virtual bool isDrawingBorder() const = 0;

		//! Sets if the button should scale the button images to fit
		virtual void setScaleImage(bool scaleImage=true) = 0;

		//! Checks whether the button scales the used images
		virtual bool isScalingImage() const = 0;
	};


} // end namespace gui
} // end namespace irr

#endif


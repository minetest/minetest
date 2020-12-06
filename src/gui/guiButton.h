// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IrrCompileConfig.h"

#include <IGUIStaticText.h>
#include "irrlicht_changes/static_text.h"
#include "IGUIButton.h"
#include "IGUISpriteBank.h"
#include "ITexture.h"
#include "SColor.h"
#include "guiSkin.h"
#include "StyleSpec.h"

using namespace irr;

#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR <= 8)
	namespace irr { namespace gui {

		//! State of buttons used for drawing texture images.
		//! Note that only a single state is active at a time
		//! Also when no image is defined for a state it will use images from another state
		//! and if that state is not set from the replacement for that,etc.
		//! So in many cases setting EGBIS_IMAGE_UP and EGBIS_IMAGE_DOWN is sufficient.
		enum EGUI_BUTTON_IMAGE_STATE {
			//! When no other states have images they will all use this one.
					EGBIS_IMAGE_UP,
			//! When not set EGBIS_IMAGE_UP is used.
					EGBIS_IMAGE_UP_MOUSEOVER,
			//! When not set EGBIS_IMAGE_UP_MOUSEOVER is used.
					EGBIS_IMAGE_UP_FOCUSED,
			//! When not set EGBIS_IMAGE_UP_FOCUSED is used.
					EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER,
			//! When not set EGBIS_IMAGE_UP is used.
					EGBIS_IMAGE_DOWN,
			//! When not set EGBIS_IMAGE_DOWN is used.
					EGBIS_IMAGE_DOWN_MOUSEOVER,
			//! When not set EGBIS_IMAGE_DOWN_MOUSEOVER is used.
					EGBIS_IMAGE_DOWN_FOCUSED,
			//! When not set EGBIS_IMAGE_DOWN_FOCUSED is used.
					EGBIS_IMAGE_DOWN_FOCUSED_MOUSEOVER,
			//! When not set EGBIS_IMAGE_UP or EGBIS_IMAGE_DOWN are used (depending on button state).
					EGBIS_IMAGE_DISABLED,
			//! not used, counts the number of enumerated items
					EGBIS_COUNT
		};

		//! Names for gui button image states
		const c8 *const GUIButtonImageStateNames[EGBIS_COUNT + 1] =
				{
						"Image",    // not "ImageUp" as it otherwise breaks serialization of old files
						"ImageUpOver",
						"ImageUpFocused",
						"ImageUpFocusedOver",
						"PressedImage",    // not "ImageDown" as it otherwise breaks serialization of old files
						"ImageDownOver",
						"ImageDownFocused",
						"ImageDownFocusedOver",
						"ImageDisabled",
						0    // count
				};

	}}

#endif

class ISimpleTextureSource;

class GUIButton : public gui::IGUIButton
{
public:

	//! constructor
	GUIButton(gui::IGUIEnvironment* environment, gui::IGUIElement* parent,
			   s32 id, core::rect<s32> rectangle, ISimpleTextureSource *tsrc,
			   bool noclip=false);

	//! destructor
	virtual ~GUIButton();

	//! called if an event happened.
	virtual bool OnEvent(const SEvent& event) override;

	//! draws the element and its children
	virtual void draw() override;

	//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
	virtual void setOverrideFont(gui::IGUIFont* font=0) override;

	//! Gets the override font (if any)
	virtual gui::IGUIFont* getOverrideFont() const override;

	//! Get the font which is used right now for drawing
	virtual gui::IGUIFont* getActiveFont() const override;

	//! Sets another color for the button text.
	virtual void setOverrideColor(video::SColor color);

	//! Gets the override color
	virtual video::SColor getOverrideColor(void) const;

	//! Sets if the button text should use the override color or the color in the gui skin.
	virtual void enableOverrideColor(bool enable);

	//! Checks if an override color is enabled
	virtual bool isOverrideColorEnabled(void) const;

	// PATCH
	//! Sets an image which should be displayed on the button when it is in the given state.
	virtual void setImage(gui::EGUI_BUTTON_IMAGE_STATE state,
			video::ITexture* image=nullptr,
			const core::rect<s32>& sourceRect=core::rect<s32>(0,0,0,0));

	//! Sets an image which should be displayed on the button when it is in normal state.
	virtual void setImage(video::ITexture* image=nullptr) override;

	//! Sets an image which should be displayed on the button when it is in normal state.
	virtual void setImage(video::ITexture* image, const core::rect<s32>& pos) override;

	//! Sets an image which should be displayed on the button when it is in pressed state.
	virtual void setPressedImage(video::ITexture* image=nullptr) override;

	//! Sets an image which should be displayed on the button when it is in pressed state.
	virtual void setPressedImage(video::ITexture* image, const core::rect<s32>& pos) override;

	//! Sets the text displayed by the button
	virtual void setText(const wchar_t* text) override;
	// END PATCH

	//! Sets the sprite bank used by the button
	virtual void setSpriteBank(gui::IGUISpriteBank* bank=0) override;

	//! Sets the animated sprite for a specific button state
	/** \param index: Number of the sprite within the sprite bank, use -1 for no sprite
	\param state: State of the button to set the sprite for
	\param index: The sprite number from the current sprite bank
	\param color: The color of the sprite
	*/
	virtual void setSprite(gui::EGUI_BUTTON_STATE state, s32 index,
						   video::SColor color=video::SColor(255,255,255,255),
						   bool loop=false, bool scale=false);

#if (IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR <= 8)
	void setSprite(gui::EGUI_BUTTON_STATE state, s32 index, video::SColor color, bool loop) override {
		setSprite(state, index, color, loop, false);
	}
#endif

	//! Get the sprite-index for the given state or -1 when no sprite is set
	virtual s32 getSpriteIndex(gui::EGUI_BUTTON_STATE state) const;

	//! Get the sprite color for the given state. Color is only used when a sprite is set.
	virtual video::SColor getSpriteColor(gui::EGUI_BUTTON_STATE state) const;

	//! Returns if the sprite in the given state does loop
	virtual bool getSpriteLoop(gui::EGUI_BUTTON_STATE state) const;

	//! Returns if the sprite in the given state is scaled
	virtual bool getSpriteScale(gui::EGUI_BUTTON_STATE state) const;

	//! Sets if the button should behave like a push button. Which means it
	//! can be in two states: Normal or Pressed. With a click on the button,
	//! the user can change the state of the button.
	virtual void setIsPushButton(bool isPushButton=true) override;

	//! Checks whether the button is a push button
	virtual bool isPushButton() const override;

	//! Sets the pressed state of the button if this is a pushbutton
	virtual void setPressed(bool pressed=true) override;

	//! Returns if the button is currently pressed
	virtual bool isPressed() const override;

	// PATCH
	//! Returns if this element (or one of its direct children) is hovered
	bool isHovered() const;
	// END PATCH

	//! Sets if the button should use the skin to draw its border
	virtual void setDrawBorder(bool border=true) override;

	//! Checks if the button face and border are being drawn
	virtual bool isDrawingBorder() const override;

	//! Sets if the alpha channel should be used for drawing images on the button (default is false)
	virtual void setUseAlphaChannel(bool useAlphaChannel=true) override;

	//! Checks if the alpha channel should be used for drawing images on the button
	virtual bool isAlphaChannelUsed() const override;

	//! Sets if the button should scale the button images to fit
	virtual void setScaleImage(bool scaleImage=true) override;

	//! Checks whether the button scales the used images
	virtual bool isScalingImage() const override;

	//! Get if the shift key was pressed in last EGET_BUTTON_CLICKED event
	virtual bool getClickShiftState() const
	{
		return ClickShiftState;
	}

	//! Get if the control key was pressed in last EGET_BUTTON_CLICKED event
	virtual bool getClickControlState() const
	{
		return ClickControlState;
	}

	//! Writes attributes of the element.
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const override;

	//! Reads attributes of the element
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options) override;



	void setColor(video::SColor color);
	// PATCH
	//! Set element properties from a StyleSpec corresponding to the button state
	void setFromState();

	//! Set element properties from a StyleSpec
	virtual void setFromStyle(const StyleSpec& style);

	//! Set the styles used for each state
	void setStyles(const std::array<StyleSpec, StyleSpec::NUM_STATES>& styles);
	// END PATCH


	//! Do not drop returned handle
	static GUIButton* addButton(gui::IGUIEnvironment *environment,
			const core::rect<s32>& rectangle, ISimpleTextureSource *tsrc,
			IGUIElement* parent, s32 id, const wchar_t* text,
			const wchar_t *tooltiptext=L"");

protected:
	void drawSprite(gui::EGUI_BUTTON_STATE state, u32 startTime, const core::position2di& center);
	gui::EGUI_BUTTON_IMAGE_STATE getImageState(bool pressed) const;

	ISimpleTextureSource *getTextureSource() { return TSrc; }

	struct ButtonImage
	{
		ButtonImage() : Texture(0), SourceRect(core::rect<s32>(0,0,0,0))
		{
		}

		ButtonImage(const ButtonImage& other) : Texture(0), SourceRect(core::rect<s32>(0,0,0,0))
		{
			*this = other;
		}

		~ButtonImage()
		{
			if ( Texture )
				Texture->drop();
		}

		ButtonImage& operator=(const ButtonImage& other)
		{
			if ( this == &other )
				return *this;

			if (other.Texture)
				other.Texture->grab();
			if ( Texture )
				Texture->drop();
			Texture = other.Texture;
			SourceRect = other.SourceRect;
			return *this;
		}

		bool operator==(const ButtonImage& other) const
		{
			return Texture == other.Texture && SourceRect == other.SourceRect;
		}


		video::ITexture* Texture;
		core::rect<s32> SourceRect;
	};

	gui::EGUI_BUTTON_IMAGE_STATE getImageState(bool pressed, const ButtonImage* images) const;

private:

	struct ButtonSprite
	{
		ButtonSprite() : Index(-1), Loop(false), Scale(false)
		{
		}

		bool operator==(const ButtonSprite& other) const
		{
			return Index == other.Index && Color == other.Color && Loop == other.Loop && Scale == other.Scale;
		}

		s32 Index;
		video::SColor Color;
		bool Loop;
		bool Scale;
	};

	ButtonSprite ButtonSprites[gui::EGBS_COUNT];
	gui::IGUISpriteBank* SpriteBank;

	ButtonImage ButtonImages[gui::EGBIS_COUNT];

	std::array<StyleSpec, StyleSpec::NUM_STATES> Styles;

	gui::IGUIFont* OverrideFont;

	bool OverrideColorEnabled;
	video::SColor OverrideColor;

	u32 ClickTime, HoverTime, FocusTime;

	bool ClickShiftState;
	bool ClickControlState;

	bool IsPushButton;
	bool Pressed;
	bool UseAlphaChannel;
	bool DrawBorder;
	bool ScaleImage;

	video::SColor Colors[4];
	// PATCH
	bool WasHovered = false;
	ISimpleTextureSource *TSrc;

	gui::IGUIStaticText *StaticText;

	core::rect<s32> BgMiddle;
	core::rect<s32> Padding;
	core::vector2d<s32> ContentOffset;
	video::SColor BgColor;
	// END PATCH
};

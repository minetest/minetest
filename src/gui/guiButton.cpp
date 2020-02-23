// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiButton.h"


#include "client/guiscalingfilter.h"
#include "client/tile.h"
#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUIFont.h"
#include "irrlicht_changes/static_text.h"
#include "porting.h"
#include "StyleSpec.h"

using namespace irr;
using namespace gui;

// Multiply with a color to get the default corresponding hovered color
#define COLOR_HOVERED_MOD 1.25f

// Multiply with a color to get the default corresponding pressed color
#define COLOR_PRESSED_MOD 0.85f

//! constructor
GUIButton::GUIButton(IGUIEnvironment* environment, IGUIElement* parent,
			s32 id, core::rect<s32> rectangle, bool noclip)
: IGUIButton(environment, parent, id, rectangle),
	SpriteBank(0), OverrideFont(0),
	OverrideColorEnabled(false), OverrideColor(video::SColor(101,255,255,255)),
	ClickTime(0), HoverTime(0), FocusTime(0),
	ClickShiftState(false), ClickControlState(false),
	IsPushButton(false), Pressed(false),
	UseAlphaChannel(false), DrawBorder(true), ScaleImage(false)
{
	setNotClipped(noclip);

	// This element can be tabbed.
	setTabStop(true);
	setTabOrder(-1);

	// PATCH
	for (size_t i = 0; i < 4; i++) {
		Colors[i] = Environment->getSkin()->getColor((EGUI_DEFAULT_COLOR)i);
		HoveredColors[i] = irr::video::SColor(Colors[i].getAlpha(),
			core::clamp<u32>(Colors[i].getRed() * COLOR_HOVERED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getGreen() * COLOR_HOVERED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getBlue() * COLOR_HOVERED_MOD, 0, 255));
		PressedColors[i] = irr::video::SColor(Colors[i].getAlpha(),
			core::clamp<u32>(Colors[i].getRed() * COLOR_PRESSED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getGreen() * COLOR_PRESSED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getBlue() * COLOR_PRESSED_MOD, 0, 255));
	}
	StaticText = gui::StaticText::add(Environment, Text.c_str(), core::rect<s32>(0,0,rectangle.getWidth(),rectangle.getHeight()), false, false, this, id);
	StaticText->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);
	// END PATCH
}

//! destructor
GUIButton::~GUIButton()
{
	if (OverrideFont)
		OverrideFont->drop();

	if (SpriteBank)
		SpriteBank->drop();
}


//! Sets if the images should be scaled to fit the button
void GUIButton::setScaleImage(bool scaleImage)
{
	ScaleImage = scaleImage;
}


//! Returns whether the button scale the used images
bool GUIButton::isScalingImage() const
{
	return ScaleImage;
}


//! Sets if the button should use the skin to draw its border
void GUIButton::setDrawBorder(bool border)
{
	DrawBorder = border;
}


void GUIButton::setSpriteBank(IGUISpriteBank* sprites)
{
	if (sprites)
		sprites->grab();

	if (SpriteBank)
		SpriteBank->drop();

	SpriteBank = sprites;
}

void GUIButton::setSprite(EGUI_BUTTON_STATE state, s32 index, video::SColor color, bool loop, bool scale)
{
	ButtonSprites[(u32)state].Index	= index;
	ButtonSprites[(u32)state].Color	= color;
	ButtonSprites[(u32)state].Loop	= loop;
	ButtonSprites[(u32)state].Scale = scale;
}

//! Get the sprite-index for the given state or -1 when no sprite is set
s32 GUIButton::getSpriteIndex(EGUI_BUTTON_STATE state) const
{
	return ButtonSprites[(u32)state].Index;
}

//! Get the sprite color for the given state. Color is only used when a sprite is set.
video::SColor GUIButton::getSpriteColor(EGUI_BUTTON_STATE state) const
{
	return ButtonSprites[(u32)state].Color;
}

//! Returns if the sprite in the given state does loop
bool GUIButton::getSpriteLoop(EGUI_BUTTON_STATE state) const
{
	return ButtonSprites[(u32)state].Loop;
}

//! Returns if the sprite in the given state is scaled
bool GUIButton::getSpriteScale(EGUI_BUTTON_STATE state) const
{
	return ButtonSprites[(u32)state].Scale;
}

//! called if an event happened.
bool GUIButton::OnEvent(const SEvent& event)
{
	if (!isEnabled())
		return IGUIElement::OnEvent(event);

	switch(event.EventType)
	{
	case EET_KEY_INPUT_EVENT:
		if (event.KeyInput.PressedDown &&
			(event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE))
		{
			if (!IsPushButton)
				setPressed(true);
			else
				setPressed(!Pressed);

			return true;
		}
		if (Pressed && !IsPushButton && event.KeyInput.PressedDown && event.KeyInput.Key == KEY_ESCAPE)
		{
			setPressed(false);
			return true;
		}
		else
		if (!event.KeyInput.PressedDown && Pressed &&
			(event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE))
		{

			if (!IsPushButton)
				setPressed(false);

			if (Parent)
			{
				ClickShiftState = event.KeyInput.Shift;
				ClickControlState = event.KeyInput.Control;

				SEvent newEvent;
				newEvent.EventType = EET_GUI_EVENT;
				newEvent.GUIEvent.Caller = this;
				newEvent.GUIEvent.Element = 0;
				newEvent.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				Parent->OnEvent(newEvent);
			}
			return true;
		}
		break;
	case EET_GUI_EVENT:
		if (event.GUIEvent.Caller == this)
		{
			if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST)
			{
				if (!IsPushButton)
					setPressed(false);
				FocusTime = (u32)porting::getTimeMs();
			}
			else if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUSED)
			{
				FocusTime = (u32)porting::getTimeMs();
			}
			else if (event.GUIEvent.EventType == EGET_ELEMENT_HOVERED || event.GUIEvent.EventType == EGET_ELEMENT_LEFT)
			{
				HoverTime = (u32)porting::getTimeMs();
			}
		}
		break;
	case EET_MOUSE_INPUT_EVENT:
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
		{
			// Sometimes formspec elements can receive mouse events when the
			// mouse is outside of the formspec. Thus, we test the position here.
			if ( !IsPushButton && AbsoluteClippingRect.isPointInside(
						core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y ))) {
				setPressed(true);
			}

			return true;
		}
		else
		if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
		{
			bool wasPressed = Pressed;

			if ( !AbsoluteClippingRect.isPointInside( core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y ) ) )
			{
				if (!IsPushButton)
					setPressed(false);
				return true;
			}

			if (!IsPushButton)
				setPressed(false);
			else
			{
				setPressed(!Pressed);
			}

			if ((!IsPushButton && wasPressed && Parent) ||
				(IsPushButton && wasPressed != Pressed))
			{
				ClickShiftState = event.MouseInput.Shift;
				ClickControlState = event.MouseInput.Control;

				SEvent newEvent;
				newEvent.EventType = EET_GUI_EVENT;
				newEvent.GUIEvent.Caller = this;
				newEvent.GUIEvent.Element = 0;
				newEvent.GUIEvent.EventType = EGET_BUTTON_CLICKED;
				Parent->OnEvent(newEvent);
			}

			return true;
		}
		break;
	default:
		break;
	}

	return Parent ? Parent->OnEvent(event) : false;
}


//! draws the element and its children
void GUIButton::draw()
{
	if (!IsVisible)
		return;

	// PATCH
	GUISkin* skin = dynamic_cast<GUISkin*>(Environment->getSkin());
	video::IVideoDriver* driver = Environment->getVideoDriver();
	// END PATCH

	if (DrawBorder)
	{
		if (!Pressed)
		{
			// PATCH
			skin->drawColored3DButtonPaneStandard(this, AbsoluteRect, &AbsoluteClippingRect,
				isHovered() ? HoveredColors : Colors);
			// END PATCH
		}
		else
		{
			// PATCH
			skin->drawColored3DButtonPanePressed(this,
					AbsoluteRect, &AbsoluteClippingRect, PressedColors);
			// END PATCH
		}
	}

	const core::position2di buttonCenter(AbsoluteRect.getCenter());
	EGUI_BUTTON_IMAGE_STATE imageState = getImageState(Pressed);
	if ( ButtonImages[(u32)imageState].Texture )
	{
		core::position2d<s32> pos(buttonCenter);
		core::rect<s32> sourceRect(ButtonImages[(u32)imageState].SourceRect);
		if ( sourceRect.getWidth() == 0 && sourceRect.getHeight() == 0 )
			sourceRect = core::rect<s32>(core::position2di(0,0), ButtonImages[(u32)imageState].Texture->getOriginalSize());

		pos.X -= sourceRect.getWidth() / 2;
		pos.Y -= sourceRect.getHeight() / 2;

		if ( Pressed )
		{
			// Create a pressed-down effect by moving the image when it looks identical to the unpressed state image
			EGUI_BUTTON_IMAGE_STATE unpressedState = getImageState(false);
			if ( unpressedState == imageState || ButtonImages[(u32)imageState] == ButtonImages[(u32)unpressedState] )
			{
				pos.X += skin->getSize(EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X);
				pos.Y += skin->getSize(EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y);
			}
		}

		// PATCH
		video::ITexture* texture = ButtonImages[(u32)imageState].Texture;
		if (BgMiddle.getArea() == 0) {
			driver->draw2DImage(texture,
					ScaleImage? AbsoluteRect : core::rect<s32>(pos, sourceRect.getSize()),
					sourceRect, &AbsoluteClippingRect,
					0, UseAlphaChannel);
		} else {
			core::rect<s32> middle = BgMiddle;
			// `-x` is interpreted as `w - x`
			if (middle.LowerRightCorner.X < 0)
				middle.LowerRightCorner.X += texture->getOriginalSize().Width;
			if (middle.LowerRightCorner.Y < 0)
				middle.LowerRightCorner.Y += texture->getOriginalSize().Height;
			draw2DImage9Slice(driver, texture,
					ScaleImage ? AbsoluteRect : core::rect<s32>(pos, sourceRect.getSize()),
					middle, &AbsoluteClippingRect);
		}
		// END PATCH
	}

	if (SpriteBank)
	{
		core::position2di pos(buttonCenter);

		if (isEnabled())
		{
			// pressed / unpressed animation
			EGUI_BUTTON_STATE state = Pressed ? EGBS_BUTTON_DOWN : EGBS_BUTTON_UP;
			drawSprite(state, ClickTime, pos);

			// focused / unfocused animation
			state = Environment->hasFocus(this) ? EGBS_BUTTON_FOCUSED : EGBS_BUTTON_NOT_FOCUSED;
			drawSprite(state, FocusTime, pos);

			// mouse over / off animation
			state = isHovered() ? EGBS_BUTTON_MOUSE_OVER : EGBS_BUTTON_MOUSE_OFF;
			drawSprite(state, HoverTime, pos);
		}
		else
		{
			// draw disabled
//			drawSprite(EGBS_BUTTON_DISABLED, 0, pos);
		}
	}

	IGUIElement::draw();
}

void GUIButton::drawSprite(EGUI_BUTTON_STATE state, u32 startTime, const core::position2di& center)
{
	u32 stateIdx = (u32)state;

	if (ButtonSprites[stateIdx].Index != -1)
	{
		if ( ButtonSprites[stateIdx].Scale )
		{
			const video::SColor colors[] = {ButtonSprites[stateIdx].Color,ButtonSprites[stateIdx].Color,ButtonSprites[stateIdx].Color,ButtonSprites[stateIdx].Color};
			SpriteBank->draw2DSprite(ButtonSprites[stateIdx].Index, AbsoluteRect.UpperLeftCorner,
					&AbsoluteClippingRect, colors[0], // FIXME: remove [0]
					porting::getTimeMs()-startTime, ButtonSprites[stateIdx].Loop);
		}
		else
		{
			SpriteBank->draw2DSprite(ButtonSprites[stateIdx].Index, center,
				&AbsoluteClippingRect, ButtonSprites[stateIdx].Color, startTime, porting::getTimeMs(),
				ButtonSprites[stateIdx].Loop, true);
		}
	}
}

EGUI_BUTTON_IMAGE_STATE GUIButton::getImageState(bool pressed) const
{
	// PATCH
	return getImageState(pressed, ButtonImages);
	// END PATCH
}

EGUI_BUTTON_IMAGE_STATE GUIButton::getImageState(bool pressed, const ButtonImage* images) const
{
	// figure state we should have
	EGUI_BUTTON_IMAGE_STATE state = EGBIS_IMAGE_DISABLED;
	bool focused = Environment->hasFocus((IGUIElement*)this);
	bool mouseOver = isHovered();
	if (isEnabled())
	{
		if ( pressed )
		{
			if ( focused && mouseOver )
				state = EGBIS_IMAGE_DOWN_FOCUSED_MOUSEOVER;
			else if ( focused )
				state = EGBIS_IMAGE_DOWN_FOCUSED;
			else if ( mouseOver )
				state = EGBIS_IMAGE_DOWN_MOUSEOVER;
			else
				state = EGBIS_IMAGE_DOWN;
		}
		else // !pressed
		{
			if ( focused && mouseOver )
				state = EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER;
			else if ( focused )
				state = EGBIS_IMAGE_UP_FOCUSED;
			else if ( mouseOver )
				state = EGBIS_IMAGE_UP_MOUSEOVER;
			else
				state = EGBIS_IMAGE_UP;
		}
	}

	// find a compatible state that has images
	while ( state != EGBIS_IMAGE_UP && !images[(u32)state].Texture )
	{
		// PATCH
		switch ( state )
		{
			case EGBIS_IMAGE_UP_FOCUSED:
				state = EGBIS_IMAGE_UP;
				break;
			case EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER:
				state = EGBIS_IMAGE_UP_FOCUSED;
				break;
			case EGBIS_IMAGE_DOWN_MOUSEOVER:
				state = EGBIS_IMAGE_DOWN;
				break;
			case EGBIS_IMAGE_DOWN_FOCUSED:
				state = EGBIS_IMAGE_DOWN;
				break;
			case EGBIS_IMAGE_DOWN_FOCUSED_MOUSEOVER:
				state = EGBIS_IMAGE_DOWN_FOCUSED;
				break;
			case EGBIS_IMAGE_DISABLED:
				if ( pressed )
					state = EGBIS_IMAGE_DOWN;
				else
					state = EGBIS_IMAGE_UP;
				break;
			default:
				state = EGBIS_IMAGE_UP;
		}
		// END PATCH
	}

	return state;
}

//! sets another skin independent font. if this is set to zero, the button uses the font of the skin.
void GUIButton::setOverrideFont(IGUIFont* font)
{
	if (OverrideFont == font)
		return;

	if (OverrideFont)
		OverrideFont->drop();

	OverrideFont = font;

	if (OverrideFont)
		OverrideFont->grab();

	StaticText->setOverrideFont(font);
}

//! Gets the override font (if any)
IGUIFont * GUIButton::getOverrideFont() const
{
	return OverrideFont;
}

//! Get the font which is used right now for drawing
IGUIFont* GUIButton::getActiveFont() const
{
	if ( OverrideFont )
		return OverrideFont;
	IGUISkin* skin = Environment->getSkin();
	if (skin)
		return skin->getFont(EGDF_BUTTON);
	return 0;
}

//! Sets another color for the text.
void GUIButton::setOverrideColor(video::SColor color)
{
	OverrideColor = color;
	OverrideColorEnabled = true;

	StaticText->setOverrideColor(color);
}

video::SColor GUIButton::getOverrideColor() const
{
	return OverrideColor;
}

void GUIButton::enableOverrideColor(bool enable)
{
	OverrideColorEnabled = enable;
}

bool GUIButton::isOverrideColorEnabled() const
{
	return OverrideColorEnabled;
}

void GUIButton::setImage(EGUI_BUTTON_IMAGE_STATE state, video::ITexture* image, const core::rect<s32>& sourceRect)
{
	if ( state >= EGBIS_COUNT )
		return;

	if ( image )
		image->grab();

	u32 stateIdx = (u32)state;
	if ( ButtonImages[stateIdx].Texture )
		ButtonImages[stateIdx].Texture->drop();

	ButtonImages[stateIdx].Texture = image;
	ButtonImages[stateIdx].SourceRect = sourceRect;
}

// PATCH
void GUIButton::setImage(video::ITexture* image)
{
	setImage(gui::EGBIS_IMAGE_UP, image);
}

void GUIButton::setImage(video::ITexture* image, const core::rect<s32>& pos)
{
	setImage(gui::EGBIS_IMAGE_UP, image, pos);
}

void GUIButton::setPressedImage(video::ITexture* image)
{
	setImage(gui::EGBIS_IMAGE_DOWN, image);
}

void GUIButton::setPressedImage(video::ITexture* image, const core::rect<s32>& pos)
{
	setImage(gui::EGBIS_IMAGE_DOWN, image, pos);
}

void GUIButton::setHoveredImage(video::ITexture* image)
{
	setImage(gui::EGBIS_IMAGE_UP_MOUSEOVER, image);
	setImage(gui::EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER, image);
}

void GUIButton::setHoveredImage(video::ITexture* image, const core::rect<s32>& pos)
{
	setImage(gui::EGBIS_IMAGE_UP_MOUSEOVER, image, pos);
	setImage(gui::EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER, image, pos);
}

//! Sets the text displayed by the button
void GUIButton::setText(const wchar_t* text)
{
	StaticText->setText(text);

	IGUIButton::setText(text);
}
// END PATCH

//! Sets if the button should behave like a push button. Which means it
//! can be in two states: Normal or Pressed. With a click on the button,
//! the user can change the state of the button.
void GUIButton::setIsPushButton(bool isPushButton)
{
	IsPushButton = isPushButton;
}


//! Returns if the button is currently pressed
bool GUIButton::isPressed() const
{
	return Pressed;
}

// PATCH
//! Returns if this element (or one of its direct children) is hovered
bool GUIButton::isHovered() const
{
	IGUIElement *hovered = Environment->getHovered();
	return  hovered == this || (hovered != nullptr && hovered->getParent() == this);
}
// END PATCH

//! Sets the pressed state of the button if this is a pushbutton
void GUIButton::setPressed(bool pressed)
{
	if (Pressed != pressed)
	{
		ClickTime = porting::getTimeMs();
		Pressed = pressed;

		GUISkin* skin = dynamic_cast<GUISkin*>(Environment->getSkin());

		for(IGUIElement *child : getChildren())
		{
			core::rect<s32> originalRect = child->getRelativePosition();
			if (Pressed) {
				child->setRelativePosition(originalRect +
						core::dimension2d<s32>(
							skin->getSize(irr::gui::EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X),
							skin->getSize(irr::gui::EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y)));
			} else {
				child->setRelativePosition(originalRect -
						core::dimension2d<s32>(
							skin->getSize(irr::gui::EGDS_BUTTON_PRESSED_IMAGE_OFFSET_X),
							skin->getSize(irr::gui::EGDS_BUTTON_PRESSED_IMAGE_OFFSET_Y)));
			}
		}
	}
}


//! Returns whether the button is a push button
bool GUIButton::isPushButton() const
{
	return IsPushButton;
}


//! Sets if the alpha channel should be used for drawing images on the button (default is false)
void GUIButton::setUseAlphaChannel(bool useAlphaChannel)
{
	UseAlphaChannel = useAlphaChannel;
}


//! Returns if the alpha channel should be used for drawing images on the button
bool GUIButton::isAlphaChannelUsed() const
{
	return UseAlphaChannel;
}


bool GUIButton::isDrawingBorder() const
{
	return DrawBorder;
}


//! Writes attributes of the element.
void GUIButton::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUIButton::serializeAttributes(out,options);

	out->addBool	("PushButton",		IsPushButton );
	if (IsPushButton)
		out->addBool("Pressed",		    Pressed);

	for ( u32 i=0; i<(u32)EGBIS_COUNT; ++i )
	{
		if ( ButtonImages[i].Texture )
		{
			core::stringc name( GUIButtonImageStateNames[i] );
			out->addTexture(name.c_str(), ButtonImages[i].Texture);
			name += "Rect";
			out->addRect(name.c_str(), ButtonImages[i].SourceRect);
		}
	}

	out->addBool	("UseAlphaChannel",	UseAlphaChannel);
	out->addBool	("Border",		    DrawBorder);
	out->addBool	("ScaleImage",		ScaleImage);

	for ( u32 i=0; i<(u32)EGBS_COUNT; ++i )
	{
		if ( ButtonSprites[i].Index >= 0 )
		{
			core::stringc nameIndex( GUIButtonStateNames[i] );
			nameIndex += "Index";
			out->addInt(nameIndex.c_str(), ButtonSprites[i].Index );

			core::stringc nameColor( GUIButtonStateNames[i] );
			nameColor += "Color";
			out->addColor(nameColor.c_str(), ButtonSprites[i].Color );

			core::stringc nameLoop( GUIButtonStateNames[i] );
			nameLoop += "Loop";
			out->addBool(nameLoop.c_str(), ButtonSprites[i].Loop );

			core::stringc nameScale( GUIButtonStateNames[i] );
			nameScale += "Scale";
			out->addBool(nameScale.c_str(), ButtonSprites[i].Scale );
		}
	}

	//   out->addString  ("OverrideFont",	OverrideFont);
}


//! Reads attributes of the element
void GUIButton::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIButton::deserializeAttributes(in,options);

	IsPushButton	= in->getAttributeAsBool("PushButton");
	Pressed		= IsPushButton ? in->getAttributeAsBool("Pressed") : false;

	core::rect<s32> rec = in->getAttributeAsRect("ImageRect");
	if (rec.isValid())
		setImage( in->getAttributeAsTexture("Image"), rec);
	else
		setImage( in->getAttributeAsTexture("Image") );

	rec = in->getAttributeAsRect("PressedImageRect");
	if (rec.isValid())
		setPressedImage( in->getAttributeAsTexture("PressedImage"), rec);
	else
		setPressedImage( in->getAttributeAsTexture("PressedImage") );

	setDrawBorder(in->getAttributeAsBool("Border"));
	setUseAlphaChannel(in->getAttributeAsBool("UseAlphaChannel"));
	setScaleImage(in->getAttributeAsBool("ScaleImage"));

	//   setOverrideFont(in->getAttributeAsString("OverrideFont"));

	updateAbsolutePosition();
}

// PATCH
GUIButton* GUIButton::addButton(IGUIEnvironment *environment, const core::rect<s32>& rectangle,
								IGUIElement* parent, s32 id, const wchar_t* text, const wchar_t *tooltiptext)
{
	GUIButton* button = new GUIButton(environment, parent ? parent : environment->getRootGUIElement(), id, rectangle);
	if (text)
		button->setText(text);

	if ( tooltiptext )
		button->setToolTipText ( tooltiptext );

	button->drop();
	return button;
}

void GUIButton::setColor(video::SColor color)
{
	float d = 0.65f;
	for (size_t i = 0; i < 4; i++) {
		video::SColor base = Environment->getSkin()->getColor((gui::EGUI_DEFAULT_COLOR)i);
		Colors[i] = base.getInterpolated(color, d);
		HoveredColors[i] = irr::video::SColor(Colors[i].getAlpha(),
			core::clamp<u32>(Colors[i].getRed() * COLOR_HOVERED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getGreen() * COLOR_HOVERED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getBlue() * COLOR_HOVERED_MOD, 0, 255));
		PressedColors[i] = irr::video::SColor(Colors[i].getAlpha(),
			core::clamp<u32>(Colors[i].getRed() * COLOR_PRESSED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getGreen() * COLOR_PRESSED_MOD, 0, 255),
			core::clamp<u32>(Colors[i].getBlue() * COLOR_PRESSED_MOD, 0, 255));
	}
}
void GUIButton::setHoveredColor(video::SColor color)
{
	float d = 0.65f;
	for (size_t i = 0; i < 4; i++) {
		video::SColor base = Environment->getSkin()->getColor((gui::EGUI_DEFAULT_COLOR)i);
		HoveredColors[i] = base.getInterpolated(color, d);
	}
}
void GUIButton::setPressedColor(video::SColor color)
{
	float d = 0.65f;
	for (size_t i = 0; i < 4; i++) {
		video::SColor base = Environment->getSkin()->getColor((gui::EGUI_DEFAULT_COLOR)i);
		PressedColors[i] = base.getInterpolated(color, d);
	}
}

//! Set element properties from a StyleSpec
void GUIButton::setFromStyle(const StyleSpec& style, ISimpleTextureSource *tsrc)
{
	if (style.isNotDefault(StyleSpec::BGCOLOR)) {
		setColor(style.getColor(StyleSpec::BGCOLOR));
	}
	if (style.isNotDefault(StyleSpec::BGCOLOR_HOVERED)) {
		setHoveredColor(style.getColor(StyleSpec::BGCOLOR_HOVERED));
	}
	if (style.isNotDefault(StyleSpec::BGCOLOR_PRESSED)) {
		setPressedColor(style.getColor(StyleSpec::BGCOLOR_PRESSED));
	}

	if (style.isNotDefault(StyleSpec::TEXTCOLOR)) {
		setOverrideColor(style.getColor(StyleSpec::TEXTCOLOR));
	}
	setNotClipped(style.getBool(StyleSpec::NOCLIP, isNotClipped()));
	setDrawBorder(style.getBool(StyleSpec::BORDER, DrawBorder));
	setUseAlphaChannel(style.getBool(StyleSpec::ALPHA, true));

	const core::position2di buttonCenter(AbsoluteRect.getCenter());
	core::position2d<s32> geom(buttonCenter);
	if (style.isNotDefault(StyleSpec::BGIMG)) {
		video::ITexture *texture = style.getTexture(StyleSpec::BGIMG, tsrc);

		setImage(guiScalingImageButton(
					Environment->getVideoDriver(), texture, geom.X, geom.Y));
		setScaleImage(true);
	}
	if (style.isNotDefault(StyleSpec::BGIMG_HOVERED)) {
		video::ITexture *hovered_texture = style.getTexture(StyleSpec::BGIMG_HOVERED, tsrc);

		setHoveredImage(guiScalingImageButton(
					Environment->getVideoDriver(), hovered_texture, geom.X, geom.Y));
		setScaleImage(true);
	}
	if (style.isNotDefault(StyleSpec::BGIMG_PRESSED)) {
		video::ITexture *pressed_texture = style.getTexture(StyleSpec::BGIMG_PRESSED, tsrc);

		setPressedImage(guiScalingImageButton(
					Environment->getVideoDriver(), pressed_texture, geom.X, geom.Y));
		setScaleImage(true);
	}
	BgMiddle = style.getRect(StyleSpec::BGIMG_MIDDLE, BgMiddle);
}
// END PATCH

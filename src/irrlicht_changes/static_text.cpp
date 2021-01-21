// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Copyright (C) 2016 Nathanaël Courant:
//   Modified the functions to use EnrichedText instead of string.
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "static_text.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include <IGUIFont.h>
#include <IVideoDriver.h>
#include <rect.h>
#include <SColor.h>
#include "util/WordWrapper.h"

#if USE_FREETYPE
	#include "CGUITTFont.h"
#endif

namespace irr
{

#if USE_FREETYPE

namespace gui
{
//! constructor
StaticText::StaticText(const EnrichedString &text, bool border,
			IGUIEnvironment* environment, IGUIElement* parent,
			s32 id, const core::rect<s32>& rectangle,
			bool background)
: IGUIStaticText(environment, parent, id, rectangle),
	HAlign(EGUIA_UPPERLEFT), VAlign(EGUIA_UPPERLEFT),
	Border(border), WordWrap(false), Background(background),
	RestrainTextInside(true), RightToLeft(false),
	OverrideFont(0), LastBreakFont(0)
{
	#ifdef _DEBUG
	setDebugName("StaticText");
	#endif

	setText(text);
}


//! destructor
StaticText::~StaticText()
{
	if (OverrideFont)
		OverrideFont->drop();
}

//! draws the element and its children
void StaticText::draw()
{
	if (!IsVisible)
		return;

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();

	core::rect<s32> frameRect(AbsoluteRect);

	// draw background

	if (Background)
		driver->draw2DRectangle(getBackgroundColor(), frameRect, &AbsoluteClippingRect);

	// draw the border

	if (Border)
	{
		skin->draw3DSunkenPane(this, 0, true, false, frameRect, &AbsoluteClippingRect);
		frameRect.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X);
		frameRect.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X);
	}

	// draw the text
	IGUIFont *font = getActiveFont();
	if (!font || BrokenText.empty()) {
		// Draw children
		IGUIElement::draw();
		return;
	}

	if (font != LastBreakFont)
		updateText();

	s32 height_line = font->getDimension(L"Ay").Height + font->getKerningHeight();

	core::rect<s32> lineRect = frameRect;

	s32 height_total = height_line * BrokenText.size();
	if (VAlign == EGUIA_CENTER)
		lineRect.UpperLeftCorner.Y = lineRect.getCenter().Y - (height_total/2);
	else if (VAlign == EGUIA_LOWERRIGHT)
		lineRect.UpperLeftCorner.Y = lineRect.LowerRightCorner.Y - height_total;

	if (HAlign == EGUIA_LOWERRIGHT)
		lineRect.UpperLeftCorner.X = lineRect.LowerRightCorner.X -
			getTextWidth();

	lineRect.LowerRightCorner.Y = lineRect.UpperLeftCorner.Y + height_line;

	irr::video::SColor previous_color(255, 255, 255, 255);
	for (const auto &str : BrokenText) {
		if (HAlign == EGUIA_LOWERRIGHT)
			lineRect.UpperLeftCorner.X = frameRect.LowerRightCorner.X -
				font->getDimension(str.c_str()).Width;

#if USE_FREETYPE
		if (font->getType() == irr::gui::EGFT_CUSTOM) {
			irr::gui::CGUITTFont *tmp = static_cast<irr::gui::CGUITTFont*>(font);
			tmp->draw(str,
					  lineRect, HAlign == EGUIA_CENTER, VAlign == EGUIA_CENTER,
					  (RestrainTextInside ? &AbsoluteClippingRect : NULL));
		} else
#endif
		{
			// Draw non-colored text
			font->draw(str.c_str(),
					   lineRect, str.getDefaultColor(), // TODO: Implement colorization
					   HAlign == EGUIA_CENTER, VAlign == EGUIA_CENTER,
					   (RestrainTextInside ? &AbsoluteClippingRect : NULL));
		}

		lineRect.LowerRightCorner.Y += height_line;
		lineRect.UpperLeftCorner.Y += height_line;
	}

	// Draw children
	IGUIElement::draw();
}

//! Sets another skin independent font.
void StaticText::setOverrideFont(IGUIFont* font)
{
	if (OverrideFont == font)
		return;

	if (OverrideFont)
		OverrideFont->drop();

	OverrideFont = font;

	if (OverrideFont)
		OverrideFont->grab();

	updateText();
}

//! Gets the override font (if any)
IGUIFont * StaticText::getOverrideFont() const
{
	return OverrideFont;
}

//! Get the font which is used right now for drawing
IGUIFont* StaticText::getActiveFont() const
{
	if ( OverrideFont )
		return OverrideFont;
	IGUISkin* skin = Environment->getSkin();
	if (skin)
		return skin->getFont();
	return 0;
}

//! Sets another color for the text.
void StaticText::setOverrideColor(video::SColor color)
{
	ColoredText.setDefaultColor(color);
	updateText();
}


//! Sets another color for the text.
void StaticText::setBackgroundColor(video::SColor color)
{
	ColoredText.setBackground(color);
	Background = true;
}


//! Sets whether to draw the background
void StaticText::setDrawBackground(bool draw)
{
	Background = draw;
}


//! Gets the background color
video::SColor StaticText::getBackgroundColor() const
{
	IGUISkin *skin = Environment->getSkin();

	return (ColoredText.hasBackground() || !skin) ?
		ColoredText.getBackground() : skin->getColor(gui::EGDC_3D_FACE);
}


//! Checks if background drawing is enabled
bool StaticText::isDrawBackgroundEnabled() const
{
	return Background;
}


//! Sets whether to draw the border
void StaticText::setDrawBorder(bool draw)
{
	Border = draw;
}


//! Checks if border drawing is enabled
bool StaticText::isDrawBorderEnabled() const
{
	return Border;
}


void StaticText::setTextRestrainedInside(bool restrainTextInside)
{
	RestrainTextInside = restrainTextInside;
}


bool StaticText::isTextRestrainedInside() const
{
	return RestrainTextInside;
}


void StaticText::setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical)
{
	HAlign = horizontal;
	VAlign = vertical;
}


video::SColor StaticText::getOverrideColor() const
{
	return ColoredText.getDefaultColor();
}

#if IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR > 8
video::SColor StaticText::getActiveColor() const
{
	return getOverrideColor();
}
#endif

//! Sets if the static text should use the overide color or the
//! color in the gui skin.
void StaticText::enableOverrideColor(bool enable)
{
	// TODO
}


bool StaticText::isOverrideColorEnabled() const
{
	return true;
}


//! Enables or disables word wrap for using the static text as
//! multiline text control.
void StaticText::setWordWrap(bool enable)
{
	WordWrap = enable;
	updateText();
}


bool StaticText::isWordWrapEnabled() const
{
	return WordWrap;
}


void StaticText::setRightToLeft(bool rtl)
{
	if (RightToLeft != rtl)
	{
		RightToLeft = rtl;
		updateText();
	}
}


bool StaticText::isRightToLeft() const
{
	return RightToLeft;
}


//! Breaks the single text line.
// Updates the font colors
void StaticText::updateText()
{
	const EnrichedString &cText = ColoredText;
	BrokenText.clear();

	if (cText.hasBackground())
		setBackgroundColor(cText.getBackground());
	else
		setDrawBackground(false);

	if (!WordWrap) {
		BrokenText.push_back(cText);
		return;
	}

	// Update word wrap

	IGUISkin* skin = Environment->getSkin();
	IGUIFont* font = getActiveFont();
	if (!font)
		return;

	LastBreakFont = font;

	core::rect<s32> bounds = { core::position2d<s32>(), RelativeRect.getSize() };
	if (Border) {
		bounds.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X);
		bounds.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X);
	}

	s32 height_line = font->getDimension(L"A").Height + font->getKerningHeight();
	WordWrapper wrapper([&](const std::wstring &str) {
		return font->getDimension(str.c_str()).Width;
	});
	wrapper.wrap(BrokenText, nullptr, cText, bounds, height_line);
}

//! Sets the new caption of this element.
void StaticText::setText(const wchar_t* text)
{
	setText(EnrichedString(text, getOverrideColor()));
}

void StaticText::setText(const EnrichedString &text)
{
	ColoredText = text;
	IGUIElement::setText(ColoredText.c_str());
	updateText();
}

void StaticText::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
	updateText();
}


//! Returns the height of the text in pixels when it is drawn.
s32 StaticText::getTextHeight() const
{
	IGUIFont* font = getActiveFont();
	if (!font)
		return 0;

	if (WordWrap) {
		s32 height = font->getDimension(L"A").Height + font->getKerningHeight();
		return height * BrokenText.size();
	}
	// There may be intentional new lines without WordWrap
	return font->getDimension(BrokenText[0].c_str()).Height;
}


s32 StaticText::getTextWidth() const
{
	IGUIFont *font = getActiveFont();
	if (!font)
		return 0;

	s32 widest = 0;

	for (const EnrichedString &line : BrokenText) {
		s32 width = font->getDimension(line.c_str()).Width;

		if (width > widest)
			widest = width;
	}

	return widest;
}


//! Writes attributes of the element.
//! Implement this to expose the attributes of your element for
//! scripting languages, editors, debuggers or xml serialization purposes.
void StaticText::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUIStaticText::serializeAttributes(out,options);

	out->addBool	("Border",              Border);
	out->addBool	("OverrideColorEnabled",true);
	out->addBool	("OverrideBGColorEnabled",ColoredText.hasBackground());
	out->addBool	("WordWrap",		WordWrap);
	out->addBool	("Background",          Background);
	out->addBool	("RightToLeft",         RightToLeft);
	out->addBool	("RestrainTextInside",  RestrainTextInside);
	out->addColor	("OverrideColor",       ColoredText.getDefaultColor());
	out->addColor	("BGColor",       	ColoredText.getBackground());
	out->addEnum	("HTextAlign",          HAlign, GUIAlignmentNames);
	out->addEnum	("VTextAlign",          VAlign, GUIAlignmentNames);

	// out->addFont ("OverrideFont",	OverrideFont);
}


//! Reads attributes of the element
void StaticText::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIStaticText::deserializeAttributes(in,options);

	Border = in->getAttributeAsBool("Border");
	setWordWrap(in->getAttributeAsBool("WordWrap"));
	Background = in->getAttributeAsBool("Background");
	RightToLeft = in->getAttributeAsBool("RightToLeft");
	RestrainTextInside = in->getAttributeAsBool("RestrainTextInside");
	if (in->getAttributeAsBool("OverrideColorEnabled"))
		ColoredText.setDefaultColor(in->getAttributeAsColor("OverrideColor"));
	if (in->getAttributeAsBool("OverrideBGColorEnabled"))
		ColoredText.setBackground(in->getAttributeAsColor("BGColor"));

	setTextAlignment( (EGUI_ALIGNMENT) in->getAttributeAsEnumeration("HTextAlign", GUIAlignmentNames),
		      (EGUI_ALIGNMENT) in->getAttributeAsEnumeration("VTextAlign", GUIAlignmentNames));

	// OverrideFont = in->getAttributeAsFont("OverrideFont");
}

} // end namespace gui

#endif // USE_FREETYPE

} // end namespace irr


#endif // _IRR_COMPILE_WITH_GUI_

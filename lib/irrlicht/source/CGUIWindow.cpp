// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIWindow.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUIButton.h"
#include "IGUIFont.h"
#include "IGUIFontBitmap.h"

namespace irr
{
namespace gui
{

//! constructor
CGUIWindow::CGUIWindow(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
: IGUIWindow(environment, parent, id, rectangle), Dragging(false), IsDraggable(true), DrawBackground(true), DrawTitlebar(true), IsActive(false)
{
	#ifdef _DEBUG
	setDebugName("CGUIWindow");
	#endif

	IGUISkin* skin = 0;
	if (environment)
		skin = environment->getSkin();

	CurrentIconColor = video::SColor(255,255,255,255);

	s32 buttonw = 15;
	if (skin)
	{
		buttonw = skin->getSize(EGDS_WINDOW_BUTTON_WIDTH);
	}
	s32 posx = RelativeRect.getWidth() - buttonw - 4;

	CloseButton = Environment->addButton(core::rect<s32>(posx, 3, posx + buttonw, 3 + buttonw), this, -1,
		L"", skin ? skin->getDefaultText(EGDT_WINDOW_CLOSE) : L"Close" );
	CloseButton->setSubElement(true);
	CloseButton->setTabStop(false);
	CloseButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	posx -= buttonw + 2;

	RestoreButton = Environment->addButton(core::rect<s32>(posx, 3, posx + buttonw, 3 + buttonw), this, -1,
		L"", skin ? skin->getDefaultText(EGDT_WINDOW_RESTORE) : L"Restore" );
	RestoreButton->setVisible(false);
	RestoreButton->setSubElement(true);
	RestoreButton->setTabStop(false);
	RestoreButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
	posx -= buttonw + 2;

	MinButton = Environment->addButton(core::rect<s32>(posx, 3, posx + buttonw, 3 + buttonw), this, -1,
		L"", skin ? skin->getDefaultText(EGDT_WINDOW_MINIMIZE) : L"Minimize" );
	MinButton->setVisible(false);
	MinButton->setSubElement(true);
	MinButton->setTabStop(false);
	MinButton->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);

	MinButton->grab();
	RestoreButton->grab();
	CloseButton->grab();

	// this element is a tab group
	setTabGroup(true);
	setTabStop(true);
	setTabOrder(-1);

	refreshSprites();
	updateClientRect();
}


//! destructor
CGUIWindow::~CGUIWindow()
{
	if (MinButton)
		MinButton->drop();

	if (RestoreButton)
		RestoreButton->drop();

	if (CloseButton)
		CloseButton->drop();
}

void CGUIWindow::refreshSprites()
{
	if (!Environment)
		return;
	IGUISkin* skin  = Environment->getSkin();
	if ( !skin )
		return;

	IGUISpriteBank* sprites = skin->getSpriteBank();
	if ( !sprites )
		return;

	CurrentIconColor = skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL);

	if (sprites)
	{
		CloseButton->setSpriteBank(sprites);
		CloseButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_WINDOW_CLOSE), CurrentIconColor);
		CloseButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_WINDOW_CLOSE), CurrentIconColor);

		RestoreButton->setSpriteBank(sprites);
		RestoreButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_WINDOW_RESTORE), CurrentIconColor);
		RestoreButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_WINDOW_RESTORE), CurrentIconColor);

		MinButton->setSpriteBank(sprites);
		MinButton->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_WINDOW_MINIMIZE), CurrentIconColor);
		MinButton->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_WINDOW_MINIMIZE), CurrentIconColor);
	}
}

//! called if an event happened.
bool CGUIWindow::OnEvent(const SEvent& event)
{
	if (isEnabled())
	{

		switch(event.EventType)
		{
		case EET_GUI_EVENT:
			if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST)
			{
				Dragging = false;
				IsActive = false;
			}
			else
			if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUSED)
			{
				if (Parent && ((event.GUIEvent.Caller == this) || isMyChild(event.GUIEvent.Caller)))
				{
					Parent->bringToFront(this);
					IsActive = true;
				}
				else
				{
					IsActive = false;
				}
			}
			else
			if (event.GUIEvent.EventType == EGET_BUTTON_CLICKED)
			{
				if (event.GUIEvent.Caller == CloseButton)
				{
					if (Parent)
					{
						// send close event to parent
						SEvent e;
						e.EventType = EET_GUI_EVENT;
						e.GUIEvent.Caller = this;
						e.GUIEvent.Element = 0;
						e.GUIEvent.EventType = EGET_ELEMENT_CLOSED;

						// if the event was not absorbed
						if (!Parent->OnEvent(e))
							remove();

						return true;

					}
					else
					{
						remove();
						return true;
					}
				}
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			switch(event.MouseInput.Event)
			{
			case EMIE_LMOUSE_PRESSED_DOWN:
				DragStart.X = event.MouseInput.X;
				DragStart.Y = event.MouseInput.Y;
				Dragging = IsDraggable;
				if (Parent)
					Parent->bringToFront(this);
				return true;
			case EMIE_LMOUSE_LEFT_UP:
				Dragging = false;
				return true;
			case EMIE_MOUSE_MOVED:
				if (!event.MouseInput.isLeftPressed())
					Dragging = false;

				if (Dragging)
				{
					// gui window should not be dragged outside its parent
					if (Parent &&
						(event.MouseInput.X < Parent->getAbsolutePosition().UpperLeftCorner.X +1 ||
							event.MouseInput.Y < Parent->getAbsolutePosition().UpperLeftCorner.Y +1 ||
							event.MouseInput.X > Parent->getAbsolutePosition().LowerRightCorner.X -1 ||
							event.MouseInput.Y > Parent->getAbsolutePosition().LowerRightCorner.Y -1))
						return true;

					move(core::position2d<s32>(event.MouseInput.X - DragStart.X, event.MouseInput.Y - DragStart.Y));
					DragStart.X = event.MouseInput.X;
					DragStart.Y = event.MouseInput.Y;
					return true;
				}
				break;
			default:
				break;
			}
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}


//! Updates the absolute position.
void CGUIWindow::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
}


//! draws the element and its children
void CGUIWindow::draw()
{
	if (IsVisible)
	{
		IGUISkin* skin = Environment->getSkin();


		// update each time because the skin is allowed to change this always.
		updateClientRect();

		if ( CurrentIconColor != skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL) )
			refreshSprites();

		core::rect<s32> rect = AbsoluteRect;

		// draw body fast
		if (DrawBackground)
		{
			rect = skin->draw3DWindowBackground(this, DrawTitlebar,
					skin->getColor(IsActive ? EGDC_ACTIVE_BORDER : EGDC_INACTIVE_BORDER),
					AbsoluteRect, &AbsoluteClippingRect);

			if (DrawTitlebar && Text.size())
			{
				rect.UpperLeftCorner.X += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_X);
				rect.UpperLeftCorner.Y += skin->getSize(EGDS_TITLEBARTEXT_DISTANCE_Y);
				rect.LowerRightCorner.X -= skin->getSize(EGDS_WINDOW_BUTTON_WIDTH) + 5;

				IGUIFont* font = skin->getFont(EGDF_WINDOW);
				if (font)
				{
					font->draw(Text.c_str(), rect,
							skin->getColor(IsActive ? EGDC_ACTIVE_CAPTION:EGDC_INACTIVE_CAPTION),
							false, true, &AbsoluteClippingRect);
				}
			}
		}
	}

	IGUIElement::draw();
}


//! Returns pointer to the close button
IGUIButton* CGUIWindow::getCloseButton() const
{
	return CloseButton;
}


//! Returns pointer to the minimize button
IGUIButton* CGUIWindow::getMinimizeButton() const
{
	return MinButton;
}


//! Returns pointer to the maximize button
IGUIButton* CGUIWindow::getMaximizeButton() const
{
	return RestoreButton;
}


//! Returns true if the window is draggable, false if not
bool CGUIWindow::isDraggable() const
{
	return IsDraggable;
}


//! Sets whether the window is draggable
void CGUIWindow::setDraggable(bool draggable)
{
	IsDraggable = draggable;

	if (Dragging && !IsDraggable)
		Dragging = false;
}


//! Set if the window background will be drawn
void CGUIWindow::setDrawBackground(bool draw)
{
	DrawBackground = draw;
}


//! Get if the window background will be drawn
bool CGUIWindow::getDrawBackground() const
{
	return DrawBackground;
}


//! Set if the window titlebar will be drawn
void CGUIWindow::setDrawTitlebar(bool draw)
{
	DrawTitlebar = draw;
}


//! Get if the window titlebar will be drawn
bool CGUIWindow::getDrawTitlebar() const
{
	return DrawTitlebar;
}


void CGUIWindow::updateClientRect()
{
	if (! DrawBackground )
	{
		ClientRect = core::rect<s32>(0,0, AbsoluteRect.getWidth(), AbsoluteRect.getHeight());
		return;
	}
	IGUISkin* skin = Environment->getSkin();
	skin->draw3DWindowBackground(this, DrawTitlebar,
			skin->getColor(IsActive ? EGDC_ACTIVE_BORDER : EGDC_INACTIVE_BORDER),
			AbsoluteRect, &AbsoluteClippingRect, &ClientRect);
	ClientRect -= AbsoluteRect.UpperLeftCorner;
}


//! Returns the rectangle of the drawable area (without border, without titlebar and without scrollbars)
core::rect<s32> CGUIWindow::getClientRect() const
{
	return ClientRect;
}


//! Writes attributes of the element.
void CGUIWindow::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUIWindow::serializeAttributes(out,options);

	out->addBool("IsDraggable", IsDraggable);
	out->addBool("DrawBackground", DrawBackground);
	out->addBool("DrawTitlebar", DrawTitlebar);

	// Currently we can't just serialize attributes of sub-elements.
	// To do this we either
	// a) allow further serialization after attribute serialiation (second function, callback or event)
	// b) add an IGUIElement attribute
	// c) extend the attribute system to allow attributes to have sub-attributes
	// We just serialize the most important info for now until we can do one of the above solutions.
	out->addBool("IsCloseVisible", CloseButton->isVisible());
	out->addBool("IsMinVisible", MinButton->isVisible());
	out->addBool("IsRestoreVisible", RestoreButton->isVisible());
}


//! Reads attributes of the element
void CGUIWindow::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
IGUIWindow::deserializeAttributes(in,options);

	Dragging = false;
	IsActive = false;
	IsDraggable = in->getAttributeAsBool("IsDraggable");
	DrawBackground = in->getAttributeAsBool("DrawBackground");
	DrawTitlebar = in->getAttributeAsBool("DrawTitlebar");

	CloseButton->setVisible(in->getAttributeAsBool("IsCloseVisible"));
	MinButton->setVisible(in->getAttributeAsBool("IsMinVisible"));
	RestoreButton->setVisible(in->getAttributeAsBool("IsRestoreVisible"));

	updateClientRect();
}


} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_


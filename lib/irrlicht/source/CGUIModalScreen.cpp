// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIModalScreen.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUIEnvironment.h"
#include "os.h"
#include "IVideoDriver.h"
#include "IGUISkin.h"

namespace irr
{
namespace gui
{

//! constructor
CGUIModalScreen::CGUIModalScreen(IGUIEnvironment* environment, IGUIElement* parent, s32 id)
: IGUIElement(EGUIET_MODAL_SCREEN, environment, parent, id, core::recti(0, 0, parent->getAbsolutePosition().getWidth(), parent->getAbsolutePosition().getHeight()) ),
	MouseDownTime(0)
{
	#ifdef _DEBUG
	setDebugName("CGUIModalScreen");
	#endif
	setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);

	// this element is a tab group
	setTabGroup(true);
}

bool CGUIModalScreen::canTakeFocus(IGUIElement* target) const
{
    return (target && ((const IGUIElement*)target == this // this element can take it
                        || isMyChild(target)    // own children also
                        || (target->getType() == EGUIET_MODAL_SCREEN )	// other modals also fine (is now on top or explicitely requested)
                        || (target->getParent() && target->getParent()->getType() == EGUIET_MODAL_SCREEN )))   // children of other modals will do
            ;
}

bool CGUIModalScreen::isVisible() const
{
    // any parent invisible?
    IGUIElement * parentElement = getParent();
    while ( parentElement )
    {
        if ( !parentElement->isVisible() )
            return false;
        parentElement = parentElement->getParent();
    }

    // if we have no children then the modal is probably abused as a way to block input
    if ( Children.empty() )
    {
        return IGUIElement::isVisible();
    }

    // any child visible?
    bool visible = false;
    core::list<IGUIElement*>::ConstIterator it = Children.begin();
    for (; it != Children.end(); ++it)
    {
        if ( (*it)->isVisible() )
        {
            visible = true;
            break;
        }
    }
    return visible;
}

bool CGUIModalScreen::isPointInside(const core::position2d<s32>& point) const
{
    return true;
}

//! called if an event happened.
bool CGUIModalScreen::OnEvent(const SEvent& event)
{
    if (!isEnabled() || !isVisible() )
        return IGUIElement::OnEvent(event);

    switch(event.EventType)
	{
	case EET_GUI_EVENT:
		switch(event.GUIEvent.EventType)
		{
		case EGET_ELEMENT_FOCUSED:
			if ( event.GUIEvent.Caller == this && isMyChild(event.GUIEvent.Element) )
			{
				Environment->removeFocus(0);	// can't setFocus otherwise at it still has focus here
				Environment->setFocus(event.GUIEvent.Element);
				MouseDownTime = os::Timer::getTime();
				return true;
			}			
			if ( !canTakeFocus(event.GUIEvent.Caller))
			{
				if ( !Children.empty() )
					Environment->setFocus(*(Children.begin()));
				else
					Environment->setFocus(this);
			}
			IGUIElement::OnEvent(event);
			return false;
		case EGET_ELEMENT_FOCUS_LOST:
			if ( !canTakeFocus(event.GUIEvent.Element))
            {
            	if ( isMyChild(event.GUIEvent.Caller) )
				{
					if ( !Children.empty() )
						Environment->setFocus(*(Children.begin()));
					else
						Environment->setFocus(this);
				}
				else
				{
					MouseDownTime = os::Timer::getTime();
				}
				return true;
			}
			else
			{
				return IGUIElement::OnEvent(event);
			}
		case EGET_ELEMENT_CLOSED:
			// do not interfere with children being removed
			return IGUIElement::OnEvent(event);
		default:
			break;
		}
		break;
	case EET_MOUSE_INPUT_EVENT:
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
		{
			MouseDownTime = os::Timer::getTime();
        }
	default:
		break;
	}

	IGUIElement::OnEvent(event);	// anyone knows why events are passed on here? Causes p.e. problems when this is child of a CGUIWindow.

	return true; // absorb everything else
}


//! draws the element and its children
void CGUIModalScreen::draw()
{
	IGUISkin *skin = Environment->getSkin();

	if (!skin)
		return;

	u32 now = os::Timer::getTime();
	if (now - MouseDownTime < 300 && (now / 70)%2)
	{
		core::list<IGUIElement*>::Iterator it = Children.begin();
		core::rect<s32> r;
		video::SColor c = Environment->getSkin()->getColor(gui::EGDC_3D_HIGH_LIGHT);

		for (; it != Children.end(); ++it)
		{
			if ((*it)->isVisible())
			{
				r = (*it)->getAbsolutePosition();
				r.LowerRightCorner.X += 1;
				r.LowerRightCorner.Y += 1;
				r.UpperLeftCorner.X -= 1;
				r.UpperLeftCorner.Y -= 1;

				skin->draw2DRectangle(this, c, r, &AbsoluteClippingRect);
			}
		}
	}

	IGUIElement::draw();
}


//! Removes a child.
void CGUIModalScreen::removeChild(IGUIElement* child)
{
	IGUIElement::removeChild(child);

	if (Children.empty())
	{
		remove();
	}
}


//! adds a child
void CGUIModalScreen::addChild(IGUIElement* child)
{
	IGUIElement::addChild(child);
	Environment->setFocus(child);
}


void CGUIModalScreen::updateAbsolutePosition()
{
	core::rect<s32> parentRect(0,0,0,0);

	if (Parent)
	{
		parentRect = Parent->getAbsolutePosition();
		RelativeRect.UpperLeftCorner.X = 0;
		RelativeRect.UpperLeftCorner.Y = 0;
		RelativeRect.LowerRightCorner.X = parentRect.getWidth();
		RelativeRect.LowerRightCorner.Y = parentRect.getHeight();
	}

	IGUIElement::updateAbsolutePosition();
}


//! Writes attributes of the element.
void CGUIModalScreen::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUIElement::serializeAttributes(out,options);
}

//! Reads attributes of the element
void CGUIModalScreen::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIElement::deserializeAttributes(in, options);
}


} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_


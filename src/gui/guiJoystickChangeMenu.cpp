// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "guiJoystickChangeMenu.h"
#include "guiButton.h"
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include <IVideoDriver.h>
#include "gettext.h"
#include "mainmenumanager.h"  // for g_gamecallback

enum
{
	GUI_ID_SAVE_BUTTON = 101,
	GUI_ID_CANCEL_BUTTON,
};

extern MainGameCallback *g_gamecallback;

GUIJoystickChangeMenu::GUIJoystickChangeMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id, IMenuManager *menumgr,
		ISimpleTextureSource *tsrc) :
		GUIModalMenu(env, parent, id, menumgr),
		m_tsrc(tsrc)
{
}

GUIJoystickChangeMenu::~GUIJoystickChangeMenu()
{
	removeAllChildren();
}

void GUIJoystickChangeMenu::regenerateGui(v2u32 screensize)
{
	removeAllChildren();

	ScalingInfo info = getScalingInfo(screensize, v2u32(835, 430));
	const float s = info.scale;
	DesiredRect = info.rect;
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	v2s32 topleft(0, 0);

	{
		core::rect<s32> rect(0, 0, 100 * s, 30 * s);
		rect += topleft + v2s32(size.X / 2 - 105 * s, size.Y - 40 * s);
		GUIButton::addButton(Environment, rect, m_tsrc, this, GUI_ID_SAVE_BUTTON,
				wstrgettext("Save").c_str());
	}

	{
		core::rect<s32> rect(0, 0, 100 * s, 30 * s);
		rect += topleft + v2s32(size.X / 2 + 5 * s, size.Y - 40 * s);
		GUIButton::addButton(Environment, rect, m_tsrc, this, GUI_ID_CANCEL_BUTTON,
				wstrgettext("Cancel").c_str());
	}
}

void GUIJoystickChangeMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();

	video::SColor bgcolor(140, 0, 0, 0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
}

bool GUIJoystickChangeMenu::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT
			&& event.KeyInput.PressedDown
			&& event.KeyInput.Key == irr::KEY_ESCAPE) {
		quitMenu();
		return true;
	}
	else if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST
			&& isVisible())
		{
			if (!canTakeFocus(event.GUIEvent.Element))
			{
				infostream << "GUIJoystickChangeMenu: Not allowing focus change."
				<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED)
		{
			switch (event.GUIEvent.Caller->getID())
			{
				case GUI_ID_SAVE_BUTTON:
					// TOOD: Save
					quitMenu();
					return true;
				case GUI_ID_CANCEL_BUTTON:
					quitMenu();
					return true;
				default:
					break;
			}
			Environment->setFocus(this);
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

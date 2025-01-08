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

#include "mainmenumanager.h"  // for g_gamecallback

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

	return Parent ? Parent->OnEvent(event) : false;
}

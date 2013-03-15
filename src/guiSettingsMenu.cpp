/*
Copyright (C) eh, you figure it out

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "guiSettingsMenu.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIScrollBar.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "main.h"

#include "gettext.h"

GUISettingsMenu::GUISettingsMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr
):
	GUIModalMenu(env, parent, id, menumgr)
{
}

GUISettingsMenu::~GUISettingsMenu()
{
	removeChildren();
}

void GUISettingsMenu::removeChildren()
{
	{
		gui::IGUIElement *e = getElementFromId(267);
		if(e != NULL)
			e->remove();
	}
}

void GUISettingsMenu::regenerateGui(v2u32 screensize)
{
	/*
		Remove stuff
	*/
	removeChildren();
	
	/*
		Calculate new sizes and positions
	*/
	core::rect<s32> rect(
			screensize.X/2 - 380/2,
			screensize.Y/2 - 200/2,
			screensize.X/2 + 380/2,
			screensize.Y/2 + 200/2
	);
	
	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 size = rect.getSize();
	v2s32 topleft_client(40, 0);
	v2s32 size_client = size - v2s32(40, 0);
	/*
		Add stuff
	*/
	changeCtype("");
	{
		core::rect<s32> rect(0, 0, 80, 30);
		rect = rect + v2s32(size.X/2-80/2, size.Y/2+55);
		Environment->addButton(rect, this, 267,
			wgettext("Exit"));
	}
	changeCtype("");
}

void GUISettingsMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);
	gui::IGUIElement::draw();
}

bool GUISettingsMenu::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
			quitMenu();
			return true;
		}
		if(event.KeyInput.Key==KEY_RETURN && event.KeyInput.PressedDown)
		{
			quitMenu();
			return true;
		}
	}
	if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			if (event.GUIEvent.Caller->getID() == 267)
				{
					quitMenu();
					return true;
				}
		}
	return Parent ? Parent->OnEvent(event) : false;
}


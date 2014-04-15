/*
Part of Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>
Copyright (C) 2013 RealBadAngel, Maciej Kasatkin <mk@realbadangel.pl>

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

#include "guiVolumeChange.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIScrollBar.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "main.h"
#include "settings.h"

#include "gettext.h"

const int ID_soundText1 = 263;
const int ID_soundText2 = 264;
const int ID_soundExitButton = 265;
const int ID_soundSlider = 266;

GUIVolumeChange::GUIVolumeChange(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		Client* client
):
	GUIModalMenu(env, parent, id, menumgr)
{
}

GUIVolumeChange::~GUIVolumeChange()
{
	removeChildren();
}

void GUIVolumeChange::removeChildren()
{
	{
		gui::IGUIElement *e = getElementFromId(ID_soundText1);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(ID_soundText2);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(ID_soundExitButton);
		if(e != NULL)
			e->remove();
	}
	{
		gui::IGUIElement *e = getElementFromId(ID_soundSlider);
		if(e != NULL)
			e->remove();
	}
}

void GUIVolumeChange::regenerateGui(v2u32 screensize)
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
	int volume=(int)(g_settings->getFloat("sound_volume")*100);
	/*
		Add stuff
	*/
	{
		core::rect<s32> rect(0, 0, 120, 20);
		rect = rect + v2s32(size.X/2-60, size.Y/2-35);
		wchar_t* text = wgettext("Sound Volume: ");
		Environment->addStaticText(text, rect, false,
				true, this, ID_soundText1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 30, 20);
		rect = rect + v2s32(size.X/2+40, size.Y/2-35);
		Environment->addStaticText(core::stringw(volume).c_str(), rect, false,
				true, this, ID_soundText2);
	}
	{
		core::rect<s32> rect(0, 0, 80, 30);
		rect = rect + v2s32(size.X/2-80/2, size.Y/2+55);
		wchar_t* text = wgettext("Exit");
		Environment->addButton(rect, this, ID_soundExitButton,
			text);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 300, 20);
		rect = rect + v2s32(size.X/2-150, size.Y/2);
		gui::IGUIScrollBar *e = Environment->addScrollBar(true,
			rect, this, ID_soundSlider);
		e->setMax(100);
		e->setPos(volume);
	}
}

void GUIVolumeChange::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);
	gui::IGUIElement::draw();
}

bool GUIVolumeChange::OnEvent(const SEvent& event)
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
			if (event.GUIEvent.Caller->getID() == ID_soundExitButton)
				{
					quitMenu();
					return true;
				}
		}
	if(event.GUIEvent.EventType==gui::EGET_SCROLL_BAR_CHANGED)
		{
		if (event.GUIEvent.Caller->getID() == ID_soundSlider)
			{
				s32 pos = ((gui::IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
				g_settings->setFloat("sound_volume",(float)pos/100);
				gui::IGUIElement *e = getElementFromId(ID_soundText2);
				e->setText( core::stringw(pos).c_str() );
				return true;
			}
		}
	return Parent ? Parent->OnEvent(event) : false;
}


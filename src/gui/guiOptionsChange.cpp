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

#include "guiOptionsChange.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIScrollBar.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "settings.h"

#include "gettext.h"

const int ID_soundText = 263;
const int ID_soundSlider = 264;
const int ID_soundMuteButton = 265;
const int ID_fovText = 266;
const int ID_fovSlider = 267;
const int ID_buildButton = 268;
const int ID_ExitButton = 269;

GUIOptionsChange::GUIOptionsChange(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr
):
	GUIModalMenu(env, parent, id, menumgr)
{
}

GUIOptionsChange::~GUIOptionsChange()
{
	removeChildren();
}

void GUIOptionsChange::removeChildren()
{
	if (gui::IGUIElement *e = getElementFromId(ID_soundText))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_ExitButton))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundSlider))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_fovText))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_fovSlider))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_buildButton))
		e->remove();
}

void GUIOptionsChange::regenerateGui(v2u32 screensize)
{
	/*
		Remove stuff
	*/
	removeChildren();

	/*
		Calculate new sizes and positions
	*/
	DesiredRect = core::rect<s32>(
		screensize.X/2 - 380/2,
		screensize.Y/2 - 400/2,
		screensize.X/2 + 380/2,
		screensize.Y/2 + 400/2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	int volume = (int)(g_settings->getFloat("sound_volume") * 100);
	int FOV = (int)(g_settings->getFloat("fov"));

	/*
		Add sound stuff
	*/
	{
		core::rect<s32> rect(0, 0, 160, 20);
		rect = rect + v2s32(size.X / 2 - 150, size.Y / 2 - 170);

		const wchar_t *text = wgettext("Sound Volume: ");
		core::stringw volume_text = text;
		delete [] text;

		volume_text += core::stringw(volume) + core::stringw("%");
		Environment->addStaticText(volume_text.c_str(), rect, false,
				true, this, ID_soundText);
	}
	{
		core::rect<s32> rect(0, 0, 160, 20);
		rect = rect + v2s32(size.X / 2 + 80, size.Y / 2 - 170);
		const wchar_t *text = wgettext("Mute");
		Environment->addCheckBox(g_settings->getBool("mute_sound"), rect, this,
				ID_soundMuteButton, text);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 300, 20);
		rect = rect + v2s32(size.X / 2 - 150, size.Y / 2-140);
		gui::IGUIScrollBar *e = Environment->addScrollBar(true,
			rect, this, ID_soundSlider);
		e->setMax(100);
		e->setPos(volume);
	}
	/* 
	    Add FOV stuff
	 */
	{
		core::rect<s32> rect(0, 0, 160, 20);
		rect = rect + v2s32(size.X / 2 - 150, size.Y / 2-110);

		const wchar_t *text = wgettext("Field of View: ");
		core::stringw fov_text = text;
		delete [] text;

		fov_text += core::stringw(FOV);
		Environment->addStaticText(fov_text.c_str(), rect, false,
				true, this, ID_fovText);
	}
	{
		core::rect<s32> rect(0, 0, 300, 20);
		rect = rect + v2s32(size.X / 2 - 150, size.Y / 2-80);
		gui::IGUIScrollBar *e = Environment->addScrollBar(true,
			rect, this, ID_fovSlider);
		e->setMax(160);
		e->setMin(51);
		e->setPos(FOV);
	}
	/*
	 * Build inside Player
	 */
	{
		core::rect<s32> rect(0, 0, 160, 20);
		rect = rect + v2s32(size.X / 2 - 150, size.Y / 2 - 50);
		const wchar_t *text = wgettext("Build Inside Player");
		Environment->addCheckBox(g_settings->getBool("enable_build_where_you_stand"), rect, this,
				ID_buildButton, text);
		delete[] text;
	}
	
	/*
		Exit Button
	 */
	{
		core::rect<s32> rect(0, 0, 80, 30);
		rect = rect + v2s32(size.X/2-80/2, size.Y/2+145);
		const wchar_t *text = wgettext("Exit");
		Environment->addButton(rect, this, ID_ExitButton,
			text);
		delete[] text;
	}
	
	
}

void GUIOptionsChange::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	video::SColor bgcolor(140, 0, 0, 0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);
	gui::IGUIElement::draw();
}

bool GUIOptionsChange::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}

		if (event.KeyInput.Key == KEY_RETURN && event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}
	} else if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_CHECKBOX_CHANGED) {
			gui::IGUIElement *e = getElementFromId(ID_soundMuteButton);
			if (e != NULL && e->getType() == gui::EGUIET_CHECK_BOX) {
				if (event.GUIEvent.Caller->getID() == ID_soundMuteButton){
					e = getElementFromId(ID_soundMuteButton);
					g_settings->setBool("mute_sound", ((gui::IGUICheckBox*)e)->isChecked());
				}else if(event.GUIEvent.Caller->getID() == ID_buildButton){
					e = getElementFromId(ID_buildButton);
					g_settings->setBool("enable_build_where_you_stand", ((gui::IGUICheckBox*)e)->isChecked());
				}
			}

			Environment->setFocus(this);
			return true;
		}
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			if (event.GUIEvent.Caller->getID() == ID_ExitButton) {
				quitMenu();
				return true;
			}
			Environment->setFocus(this);
		}

		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible()) {
			if (!canTakeFocus(event.GUIEvent.Element)) {
				dstream << "GUIMainMenu: Not allowing focus change."
				<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED) {
			if (event.GUIEvent.Caller->getID() == ID_soundSlider) {
				s32 pos = ((gui::IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
				g_settings->setFloat("sound_volume", (float) pos / 100);

				gui::IGUIElement *e = getElementFromId(ID_soundText);
				const wchar_t *text = wgettext("Sound Volume: ");
				core::stringw volume_text = text;
				delete [] text;

				volume_text += core::stringw(pos) + core::stringw("%");
				e->setText(volume_text.c_str());
				return true;
			}else if (event.GUIEvent.Caller->getID() == ID_fovSlider) {
				s32 pos = ((gui::IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
				g_settings->setFloat("fov", (float) pos);

				gui::IGUIElement *e = getElementFromId(ID_fovText);
				const wchar_t *text = wgettext("Field of View: ");
				core::stringw fov_text = text;
				delete [] text;

				fov_text += core::stringw(pos);
				e->setText(fov_text.c_str());
				return true;
			}
		}

	}

	return Parent ? Parent->OnEvent(event) : false;
}


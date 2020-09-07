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
#include "guiButton.h"
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
const int ID_soundExitButton = 264;
const int ID_soundSlider = 265;
const int ID_soundMuteButton = 266;

GUIVolumeChange::GUIVolumeChange(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr, ISimpleTextureSource *tsrc
):
	GUIModalMenu(env, parent, id, menumgr),
	m_tsrc(tsrc)
{
}

GUIVolumeChange::~GUIVolumeChange()
{
	removeChildren();
}

void GUIVolumeChange::removeChildren()
{
	if (gui::IGUIElement *e = getElementFromId(ID_soundText))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundExitButton))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundSlider))
		e->remove();

	if (gui::IGUIElement *e = getElementFromId(ID_soundMuteButton))
		e->remove();
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
	const float s = m_gui_scale;
	DesiredRect = core::rect<s32>(
		screensize.X / 2 - 380 * s / 2,
		screensize.Y / 2 - 200 * s / 2,
		screensize.X / 2 + 380 * s / 2,
		screensize.Y / 2 + 200 * s / 2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	int volume = (int)(g_settings->getFloat("sound_volume") * 100);

	/*
		Add stuff
	*/
	{
		core::rect<s32> rect(0, 0, 160 * s, 20 * s);
		rect = rect + v2s32(size.X / 2 - 80 * s, size.Y / 2 - 70 * s);

		const wchar_t *text = wgettext("Sound Volume: ");
		core::stringw volume_text = text;
		delete [] text;

		volume_text += core::stringw(volume) + core::stringw("%");
		Environment->addStaticText(volume_text.c_str(), rect, false,
				true, this, ID_soundText);
	}
	{
		core::rect<s32> rect(0, 0, 80 * s, 30 * s);
		rect = rect + v2s32(size.X / 2 - 80 * s / 2, size.Y / 2 + 55 * s);
		const wchar_t *text = wgettext("Exit");
		GUIButton::addButton(Environment, rect, m_tsrc, this, ID_soundExitButton, text);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 300 * s, 20 * s);
		rect = rect + v2s32(size.X / 2 - 150 * s, size.Y / 2);
		gui::IGUIScrollBar *e = Environment->addScrollBar(true,
			rect, this, ID_soundSlider);
		e->setMax(100);
		e->setPos(volume);
	}
	{
		core::rect<s32> rect(0, 0, 160 * s, 20 * s);
		rect = rect + v2s32(size.X / 2 - 80 * s, size.Y / 2 - 35 * s);
		const wchar_t *text = wgettext("Muted");
		Environment->addCheckBox(g_settings->getBool("mute_sound"), rect, this,
				ID_soundMuteButton, text);
		delete[] text;
	}
}

void GUIVolumeChange::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	video::SColor bgcolor(140, 0, 0, 0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);
	gui::IGUIElement::draw();
}

bool GUIVolumeChange::OnEvent(const SEvent& event)
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
				g_settings->setBool("mute_sound", ((gui::IGUICheckBox*)e)->isChecked());
			}

			Environment->setFocus(this);
			return true;
		}

		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			if (event.GUIEvent.Caller->getID() == ID_soundExitButton) {
				quitMenu();
				return true;
			}
			Environment->setFocus(this);
		}

		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible()) {
			if (!canTakeFocus(event.GUIEvent.Element)) {
				infostream << "GUIVolumeChange: Not allowing focus change."
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
			}
		}

	}

	return Parent ? Parent->OnEvent(event) : false;
}

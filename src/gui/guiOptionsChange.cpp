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
const int ID_forwardButton = 269;
const int ID_ExitButton = 271;

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

	if (gui::IGUIElement *e = getElementFromId(ID_forwardButton))
		e->remove();
}

//for streamlining the GUI process and cleaning up code
void GUIOptionsChange::addCheckBox(const std::string name,const std::string setting, int ID, int xoff, int yoff){
	core::rect<s32> rect(0, 0, 160, 20);
	rect = rect + v2s32(m_size.X / 2 + xoff, m_size.Y / 2 + yoff);
	const wchar_t *text = wgettext(name.c_str());
	Environment->addCheckBox(g_settings->getBool(setting), rect, this,
			ID, text);
	delete[] text;
}

void GUIOptionsChange::addSlider(int ID, int xoff, int yoff, int max, int min, int init){
	core::rect<s32> rect(0, 0, 300, 20);
	rect = rect + v2s32(m_size.X / 2 + xoff, m_size.Y / 2 + yoff);
	gui::IGUIScrollBar *e = Environment->addScrollBar(true,
		rect, this, ID);
	e->setMax(max);
	e->setMin(min);
	e->setPos(init);
}

void GUIOptionsChange::addDynText(const std::string name, int value, const std::string ending, int ID, int xoff, int yoff){
	core::rect<s32> rect(0, 0, 160, 20);
	rect = rect + v2s32(m_size.X / 2 + xoff, m_size.Y / 2 + yoff);

	const wchar_t *text = wgettext(name.c_str());
	core::stringw temp_text = text;
	delete[] text;

	temp_text += core::stringw(value)+core::stringw(wgettext(ending.c_str()));
	Environment->addStaticText(temp_text.c_str(), rect, false,
			true, this, ID);
}

void GUIOptionsChange::addText(const std::string name, int ID, int xoff, int yoff){
	core::rect<s32> rect(0, 0, 160, 20);
	rect = rect + v2s32(m_size.X / 2 + xoff, m_size.Y / 2 + yoff);

	const wchar_t *text = wgettext(name.c_str());
	core::stringw temp_text = text;
	delete[] text;
	
	Environment->addStaticText(temp_text.c_str(), rect, false,
			true, this, ID);
}

void GUIOptionsChange::updateDynText(const SEvent& event, const std::string setting, int scale, int ID, const std::string beginning, const std::string ending){
	s32 pos = ((gui::IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
	g_settings->setFloat(setting, (float) pos / scale);

	gui::IGUIElement *e = getElementFromId(ID);
	const wchar_t *text = wgettext(beginning.c_str());
	core::stringw temp_text = text;
	delete[] text;

	temp_text += core::stringw(pos) + core::stringw(ending.c_str());
	e->setText(temp_text.c_str());
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

	m_size = DesiredRect.getSize();
	v2s32 size = m_size;
	int volume = (int)(g_settings->getFloat("sound_volume") * 100);
	int FOV = (int)(g_settings->getFloat("fov"));

	//sound stuff
	addDynText("Sound Volume: ",volume,"%",ID_soundText,-150,-170);
	addCheckBox("Mute","mute_sound",ID_soundMuteButton,80,-170);
	addSlider(ID_soundSlider,-150,-140,100,0,volume);
	//fov stuff
	addDynText("Field Of View: ",FOV,"",ID_fovText,-150,-110);
	addSlider(ID_fovSlider,-150,-80, 160, 51, FOV);
	//misc settings
	addCheckBox("Build Inside Player","enable_build_where_you_stand",ID_buildButton,-150,-50);
	addCheckBox("Auto-Forward","continuous_forward",ID_forwardButton,-150,-20);
	//exit button
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
			// check boxes
			if (e != NULL && e->getType() == gui::EGUIET_CHECK_BOX) {
				if (event.GUIEvent.Caller->getID() == ID_soundMuteButton){
					e = getElementFromId(ID_soundMuteButton);
					g_settings->setBool("mute_sound", ((gui::IGUICheckBox*)e)->isChecked());
				}else if(event.GUIEvent.Caller->getID() == ID_buildButton){
					e = getElementFromId(ID_buildButton);
					g_settings->setBool("enable_build_where_you_stand", ((gui::IGUICheckBox*)e)->isChecked());
				}else if(event.GUIEvent.Caller->getID() == ID_forwardButton){
					e = getElementFromId(ID_forwardButton);
					g_settings->setBool("continuous_forward", ((gui::IGUICheckBox*)e)->isChecked());
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
			//sliders
			if (event.GUIEvent.Caller->getID() == ID_soundSlider) {
				updateDynText(event, "sound_volume", 100, ID_soundText, "Sound Volume: ", "%");
				return true;
			}else if (event.GUIEvent.Caller->getID() == ID_fovSlider) {
				updateDynText(event, "fov", 1, ID_fovText, "Field Of View: ", "");
				return true;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}


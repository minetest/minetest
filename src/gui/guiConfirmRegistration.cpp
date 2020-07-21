/*
Minetest
Copyright (C) 2018 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "guiConfirmRegistration.h"
#include "client/client.h"
#include "guiButton.h"
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "intlGUIEditBox.h"
#include "porting.h"

#include "gettext.h"

// Continuing from guiPasswordChange.cpp
const int ID_confirmPassword = 262;
const int ID_confirm = 263;
const int ID_intotext = 264;
const int ID_cancel = 265;
const int ID_message = 266;

GUIConfirmRegistration::GUIConfirmRegistration(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, IMenuManager *menumgr, Client *client,
		const std::string &playername, const std::string &password,
		bool *aborted, ISimpleTextureSource *tsrc) :
		GUIModalMenu(env, parent, id, menumgr),
		m_client(client), m_playername(playername), m_password(password),
		m_aborted(aborted), m_tsrc(tsrc)
{
#ifdef __ANDROID__
	m_touchscreen_visible = false;
#endif
}

GUIConfirmRegistration::~GUIConfirmRegistration()
{
	removeChildren();
}

void GUIConfirmRegistration::removeChildren()
{
	const core::list<gui::IGUIElement *> &children = getChildren();
	core::list<gui::IGUIElement *> children_copy;
	for (gui::IGUIElement *i : children)
		children_copy.push_back(i);
	for (gui::IGUIElement *i : children_copy)
		i->remove();
}

void GUIConfirmRegistration::regenerateGui(v2u32 screensize)
{
	acceptInput();
	removeChildren();

	/*
		Calculate new sizes and positions
	*/
#ifdef __ANDROID__
	const float s = m_gui_scale * porting::getDisplayDensity() / 2;
#else
	const float s = m_gui_scale;
#endif
	DesiredRect = core::rect<s32>(
		screensize.X / 2 - 600 * s / 2,
		screensize.Y / 2 - 360 * s / 2,
		screensize.X / 2 + 600 * s / 2,
		screensize.Y / 2 + 360 * s / 2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	v2s32 topleft_client(0, 0);

	const wchar_t *text;

	/*
		Add stuff
	*/
	s32 ypos = 30 * s;
	{
		core::rect<s32> rect2(0, 0, 540 * s, 180 * s);
		rect2 += topleft_client + v2s32(30 * s, ypos);
		static const std::string info_text_template = strgettext(
				"You are about to join this server with the name \"%s\" for the "
				"first time.\n"
				"If you proceed, a new account using your credentials will be "
				"created on this server.\n"
				"Please retype your password and click 'Register and Join' to "
				"confirm account creation, or click 'Cancel' to abort.");
		char info_text_buf[1024];
		porting::mt_snprintf(info_text_buf, sizeof(info_text_buf),
				info_text_template.c_str(), m_playername.c_str());

		wchar_t *info_text_buf_wide = utf8_to_wide_c(info_text_buf);
		gui::IGUIEditBox *e = new gui::intlGUIEditBox(info_text_buf_wide, true,
				Environment, this, ID_intotext, rect2, false, true);
		delete[] info_text_buf_wide;
		e->drop();
		e->setMultiLine(true);
		e->setWordWrap(true);
		e->setTextAlignment(gui::EGUIA_UPPERLEFT, gui::EGUIA_CENTER);
	}

	ypos += 200 * s;
	{
		core::rect<s32> rect2(0, 0, 540 * s, 30 * s);
		rect2 += topleft_client + v2s32(30 * s, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(m_pass_confirm.c_str(),
				rect2, true, this, ID_confirmPassword);
		e->setPasswordBox(true);
		Environment->setFocus(e);
	}

	ypos += 50 * s;
	{
		core::rect<s32> rect2(0, 0, 230 * s, 35 * s);
		rect2 = rect2 + v2s32(size.X / 2 - 220 * s, ypos);
		text = wgettext("Register and Join");
		GUIButton::addButton(Environment, rect2, m_tsrc, this, ID_confirm, text);
		delete[] text;
	}
	{
		core::rect<s32> rect2(0, 0, 120 * s, 35 * s);
		rect2 = rect2 + v2s32(size.X / 2 + 70 * s, ypos);
		text = wgettext("Cancel");
		GUIButton::addButton(Environment, rect2, m_tsrc, this, ID_cancel, text);
		delete[] text;
	}
	{
		core::rect<s32> rect2(0, 0, 500 * s, 40 * s);
		rect2 += topleft_client + v2s32(30 * s, ypos + 40 * s);
		text = wgettext("Passwords do not match!");
		IGUIElement *e = Environment->addStaticText(
				text, rect2, false, true, this, ID_message);
		e->setVisible(false);
		delete[] text;
	}
}

void GUIConfirmRegistration::drawMenu()
{
	gui::IGUISkin *skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver *driver = Environment->getVideoDriver();

	video::SColor bgcolor(140, 0, 0, 0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	gui::IGUIElement::draw();
#ifdef __ANDROID__
	getAndroidUIInput();
#endif
}

void GUIConfirmRegistration::closeMenu(bool goNext)
{
	if (goNext) {
		m_client->confirmRegistration();
	} else {
		*m_aborted = true;
		infostream << "Connect aborted [Escape]" << std::endl;
	}
	quitMenu();
}

void GUIConfirmRegistration::acceptInput()
{
	gui::IGUIElement *e;
	e = getElementFromId(ID_confirmPassword);
	if (e)
		m_pass_confirm = e->getText();
}

bool GUIConfirmRegistration::processInput()
{
	std::wstring m_password_ws = narrow_to_wide(m_password);
	if (m_password_ws != m_pass_confirm) {
		gui::IGUIElement *e = getElementFromId(ID_message);
		if (e)
			e->setVisible(true);
		return false;
	}
	return true;
}

bool GUIConfirmRegistration::OnEvent(const SEvent &event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		// clang-format off
		if ((event.KeyInput.Key == KEY_ESCAPE ||
				event.KeyInput.Key == KEY_CANCEL) &&
				event.KeyInput.PressedDown) {
			closeMenu(false);
			return true;
		}
		// clang-format on
		if (event.KeyInput.Key == KEY_RETURN && event.KeyInput.PressedDown) {
			acceptInput();
			if (processInput())
				closeMenu(true);
			return true;
		}
	}

	if (event.EventType != EET_GUI_EVENT)
		return Parent ? Parent->OnEvent(event) : false;

	if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST && isVisible()) {
		if (!canTakeFocus(event.GUIEvent.Element)) {
			infostream << "GUIConfirmRegistration: Not allowing focus change."
				<< std::endl;
			// Returning true disables focus change
			return true;
		}
	} else if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
		switch (event.GUIEvent.Caller->getID()) {
		case ID_confirm:
			acceptInput();
			if (processInput())
				closeMenu(true);
			return true;
		case ID_cancel:
			closeMenu(false);
			return true;
		}
	} else if (event.GUIEvent.EventType == gui::EGET_EDITBOX_ENTER) {
		switch (event.GUIEvent.Caller->getID()) {
		case ID_confirmPassword:
			acceptInput();
			if (processInput())
				closeMenu(true);
			return true;
		}
	}

	return false;
}

#ifdef __ANDROID__
bool GUIConfirmRegistration::getAndroidUIInput()
{
	if (!hasAndroidUIInput() || m_jni_field_name != "password")
		return false;

	// still waiting
	if (porting::getInputDialogState() == -1)
		return true;

	m_jni_field_name.clear();

	gui::IGUIElement *e = getElementFromId(ID_confirmPassword);

	if (!e || e->getType() != irr::gui::EGUIET_EDIT_BOX)
		return false;

	std::string text = porting::getInputDialogValue();
	e->setText(utf8_to_wide(text).c_str());
	return false;
}
#endif

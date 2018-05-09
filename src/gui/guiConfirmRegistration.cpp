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
#include "client.h"
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "intlGUIEditBox.h"

#include "gettext.h"

// Continuing from guiPasswordChange.cpp
const int ID_confirmPassword = 262;
const int ID_confirm = 263;
const int ID_message = 264;
const int ID_cancel = 265;

GUIConfirmRegistration::GUIConfirmRegistration(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, IMenuManager *menumgr, Client *client,
		const std::string &playername, const std::string &password,
		const std::string &address, bool *aborted) :
		GUIModalMenu(env, parent, id, menumgr),
		m_client(client), m_playername(playername), m_password(password),
		m_address(address), m_aborted(aborted)
{
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
	core::rect<s32> rect(screensize.X / 2 - 600 / 2, screensize.Y / 2 - 360 / 2,
			screensize.X / 2 + 600 / 2, screensize.Y / 2 + 360 / 2);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 size = rect.getSize();
	v2s32 topleft_client(0, 0);

	const wchar_t *text;

	/*
		Add stuff
	*/
	s32 ypos = 30;
	{
		std::string address = m_address;
		if (address.empty())
			address = "localhost";
		core::rect<s32> rect2(0, 0, 540, 180);
		rect2 += topleft_client + v2s32(30, ypos);
		static const std::string info_text_template = strgettext(
				"You are about to join the server at %1$s with the "
				"name \"%2$s\" for the first time. If you proceed, a "
				"new account using your credentials will be created "
				"on this server.\n"
				"Please retype your password and click Register and "
				"Join to confirm account creation or click Cancel to "
				"abort.");
		char info_text_buf[1024];
		snprintf(info_text_buf, sizeof(info_text_buf), info_text_template.c_str(),
				address.c_str(), m_playername.c_str());

		gui::IGUIEditBox *e = new gui::intlGUIEditBox(
				utf8_to_wide_c(info_text_buf), true, Environment, this,
				ID_message, rect2, false, true);
		e->drop();
		e->setMultiLine(true);
		e->setWordWrap(true);
		e->setTextAlignment(gui::EGUIA_UPPERLEFT, gui::EGUIA_CENTER);
	}

	ypos += 210;
	{
		core::rect<s32> rect2(0, 0, 540, 30);
		rect2 += topleft_client + v2s32(30, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(m_pass_confirm.c_str(),
				rect2, true, this, ID_confirmPassword);
		e->setPasswordBox(true);
	}

	ypos += 60;
	{
		core::rect<s32> rect2(0, 0, 230, 35);
		rect2 = rect2 + v2s32(size.X / 2 - 220, ypos);
		text = wgettext("Register and Join");
		Environment->addButton(rect2, this, ID_confirm, text);
		delete[] text;
	}
	{
		core::rect<s32> rect2(0, 0, 120, 35);
		rect2 = rect2 + v2s32(size.X / 2 + 70, ypos);
		text = wgettext("Cancel");
		Environment->addButton(rect2, this, ID_cancel, text);
		delete[] text;
	}
	{
		core::rect<s32> rect2(0, 0, 200, 20);
		rect2 += topleft_client + v2s32(30, ypos - 40);
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
		if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown) {
			closeMenu(false);
			return true;
		}
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
			dstream << "GUIConfirmRegistration: Not allowing focus "
				   "change."
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

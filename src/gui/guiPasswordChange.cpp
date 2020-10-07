/*
Part of Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Ciaran Gultnieks <ciaran@ciarang.com>

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

#include "guiPasswordChange.h"
#include "client/client.h"
#include "guiButton.h"
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>

#include "porting.h"
#include "gettext.h"

const int ID_oldPassword = 256;
const int ID_newPassword1 = 257;
const int ID_newPassword2 = 258;
const int ID_change = 259;
const int ID_message = 260;
const int ID_cancel = 261;

GUIPasswordChange::GUIPasswordChange(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		Client* client,
		ISimpleTextureSource *tsrc
):
	GUIModalMenu(env, parent, id, menumgr),
	m_client(client),
	m_tsrc(tsrc)
{
}

GUIPasswordChange::~GUIPasswordChange()
{
	removeChildren();
}

void GUIPasswordChange::removeChildren()
{
	const core::list<gui::IGUIElement *> &children = getChildren();
	core::list<gui::IGUIElement *> children_copy;
	for (gui::IGUIElement *i : children) {
		children_copy.push_back(i);
	}

	for (gui::IGUIElement *i : children_copy) {
		i->remove();
	}
}
void GUIPasswordChange::regenerateGui(v2u32 screensize)
{
	/*
		save current input
	*/
	acceptInput();

	/*
		Remove stuff
	*/
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
		screensize.X / 2 - 580 * s / 2,
		screensize.Y / 2 - 300 * s / 2,
		screensize.X / 2 + 580 * s / 2,
		screensize.Y / 2 + 300 * s / 2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	v2s32 topleft_client(40 * s, 0);

	const wchar_t *text;

	/*
		Add stuff
	*/
	s32 ypos = 50 * s;
	{
		core::rect<s32> rect(0, 0, 150 * s, 20 * s);
		rect += topleft_client + v2s32(25 * s, ypos + 6 * s);
		text = wgettext("Old Password");
		Environment->addStaticText(text, rect, false, true, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 230 * s, 30 * s);
		rect += topleft_client + v2s32(160 * s, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(
				m_oldpass.c_str(), rect, true, this, ID_oldPassword);
		Environment->setFocus(e);
		e->setPasswordBox(true);
	}
	ypos += 50 * s;
	{
		core::rect<s32> rect(0, 0, 150 * s, 20 * s);
		rect += topleft_client + v2s32(25 * s, ypos + 6 * s);
		text = wgettext("New Password");
		Environment->addStaticText(text, rect, false, true, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 230 * s, 30 * s);
		rect += topleft_client + v2s32(160 * s, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(
				m_newpass.c_str(), rect, true, this, ID_newPassword1);
		e->setPasswordBox(true);
	}
	ypos += 50 * s;
	{
		core::rect<s32> rect(0, 0, 150 * s, 20 * s);
		rect += topleft_client + v2s32(25 * s, ypos + 6 * s);
		text = wgettext("Confirm Password");
		Environment->addStaticText(text, rect, false, true, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 230 * s, 30 * s);
		rect += topleft_client + v2s32(160 * s, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(
				m_newpass_confirm.c_str(), rect, true, this, ID_newPassword2);
		e->setPasswordBox(true);
	}

	ypos += 50 * s;
	{
		core::rect<s32> rect(0, 0, 100 * s, 30 * s);
		rect = rect + v2s32(size.X / 4 + 56 * s, ypos);
		text = wgettext("Change");
		GUIButton::addButton(Environment, rect, m_tsrc, this, ID_change, text);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 100 * s, 30 * s);
		rect = rect + v2s32(size.X / 4 + 185 * s, ypos);
		text = wgettext("Cancel");
		GUIButton::addButton(Environment, rect, m_tsrc, this, ID_cancel, text);
		delete[] text;
	}

	ypos += 50 * s;
	{
		core::rect<s32> rect(0, 0, 300 * s, 20 * s);
		rect += topleft_client + v2s32(35 * s, ypos);
		text = wgettext("Passwords do not match!");
		IGUIElement *e =
			Environment->addStaticText(
			text, rect, false, true, this, ID_message);
		e->setVisible(false);
		delete[] text;
	}
}

void GUIPasswordChange::drawMenu()
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

void GUIPasswordChange::acceptInput()
{
	gui::IGUIElement *e;
	e = getElementFromId(ID_oldPassword);
	if (e != NULL)
		m_oldpass = e->getText();
	e = getElementFromId(ID_newPassword1);
	if (e != NULL)
		m_newpass = e->getText();
	e = getElementFromId(ID_newPassword2);
	if (e != NULL)
		m_newpass_confirm = e->getText();
}

bool GUIPasswordChange::processInput()
{
	if (m_newpass != m_newpass_confirm) {
		gui::IGUIElement *e = getElementFromId(ID_message);
		if (e != NULL)
			e->setVisible(true);
		return false;
	}
	m_client->sendChangePassword(wide_to_utf8(m_oldpass), wide_to_utf8(m_newpass));
	return true;
}

bool GUIPasswordChange::OnEvent(const SEvent &event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		// clang-format off
		if ((event.KeyInput.Key == KEY_ESCAPE ||
				event.KeyInput.Key == KEY_CANCEL) &&
				event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}
		// clang-format on
		if (event.KeyInput.Key == KEY_RETURN && event.KeyInput.PressedDown) {
			acceptInput();
			if (processInput())
				quitMenu();
			return true;
		}
	}
	if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST &&
				isVisible()) {
			if (!canTakeFocus(event.GUIEvent.Element)) {
				infostream << "GUIPasswordChange: Not allowing focus change."
					<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			switch (event.GUIEvent.Caller->getID()) {
			case ID_change:
				acceptInput();
				if (processInput())
					quitMenu();
				return true;
			case ID_cancel:
				quitMenu();
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_EDITBOX_ENTER) {
			switch (event.GUIEvent.Caller->getID()) {
			case ID_oldPassword:
			case ID_newPassword1:
			case ID_newPassword2:
				acceptInput();
				if (processInput())
					quitMenu();
				return true;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

std::string GUIPasswordChange::getNameByID(s32 id)
{
	switch (id) {
	case ID_oldPassword:
		return "old_password";
	case ID_newPassword1:
		return "new_password_1";
	case ID_newPassword2:
		return "new_password_2";
	}
	return "";
}

#ifdef __ANDROID__
bool GUIPasswordChange::getAndroidUIInput()
{
	if (!hasAndroidUIInput())
		return false;

	// still waiting
	if (porting::getInputDialogState() == -1)
		return true;

	gui::IGUIElement *e = nullptr;
	if (m_jni_field_name == "old_password")
		e = getElementFromId(ID_oldPassword);
	else if (m_jni_field_name == "new_password_1")
		e = getElementFromId(ID_newPassword1);
	else if (m_jni_field_name == "new_password_2")
		e = getElementFromId(ID_newPassword2);
	m_jni_field_name.clear();

	if (!e || e->getType() != irr::gui::EGUIET_EDIT_BOX)
		return false;

	std::string text = porting::getInputDialogValue();
	e->setText(utf8_to_wide(text).c_str());
	return false;
}
#endif

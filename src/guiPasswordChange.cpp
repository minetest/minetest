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
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>

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
		Client* client
):
	GUIModalMenu(env, parent, id, menumgr),
	m_client(client),
	m_oldpass(L""),
	m_newpass(L""),
	m_newpass_confirm(L"")
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
	for (core::list<gui::IGUIElement *>::ConstIterator i = children.begin();
			i != children.end(); i++) {
		children_copy.push_back(*i);
	}
	for (core::list<gui::IGUIElement *>::Iterator i = children_copy.begin();
			i != children_copy.end(); i++) {
		(*i)->remove();
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
	core::rect<s32> rect(
			screensize.X/2 - 580/2,
			screensize.Y/2 - 300/2,
			screensize.X/2 + 580/2,
			screensize.Y/2 + 300/2
	);

	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 size = rect.getSize();
	v2s32 topleft_client(40, 0);

	const wchar_t *text;

	/*
		Add stuff
	*/
	s32 ypos = 50;
	{
		core::rect<s32> rect(0, 0, 150, 20);
		rect += topleft_client + v2s32(25, ypos + 6);
		text = wgettext("Old Password");
		Environment->addStaticText(text, rect, false, true, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 230, 30);
		rect += topleft_client + v2s32(160, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(
				m_oldpass.c_str(), rect, true, this, ID_oldPassword);
		Environment->setFocus(e);
		e->setPasswordBox(true);
	}
	ypos += 50;
	{
		core::rect<s32> rect(0, 0, 150, 20);
		rect += topleft_client + v2s32(25, ypos + 6);
		text = wgettext("New Password");
		Environment->addStaticText(text, rect, false, true, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 230, 30);
		rect += topleft_client + v2s32(160, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(
				m_newpass.c_str(), rect, true, this, ID_newPassword1);
		e->setPasswordBox(true);
	}
	ypos += 50;
	{
		core::rect<s32> rect(0, 0, 150, 20);
		rect += topleft_client + v2s32(25, ypos + 6);
		text = wgettext("Confirm Password");
		Environment->addStaticText(text, rect, false, true, this, -1);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 230, 30);
		rect += topleft_client + v2s32(160, ypos);
		gui::IGUIEditBox *e = Environment->addEditBox(
				m_newpass_confirm.c_str(), rect, true, this, ID_newPassword2);
		e->setPasswordBox(true);
	}

	ypos += 50;
	{
		core::rect<s32> rect(0, 0, 100, 30);
		rect = rect + v2s32(size.X / 4 + 56, ypos);
		text = wgettext("Change");
		Environment->addButton(rect, this, ID_change, text);
		delete[] text;
	}
	{
		core::rect<s32> rect(0, 0, 100, 30);
		rect = rect + v2s32(size.X / 4 + 185, ypos);
		text = wgettext("Cancel");
		Environment->addButton(rect, this, ID_cancel, text);
		delete[] text;
	}

	ypos += 50;
	{
		core::rect<s32> rect(0, 0, 300, 20);
		rect += topleft_client + v2s32(35, ypos);
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
		if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}
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
				dstream << "GUIPasswordChange: Not allowing focus change."
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

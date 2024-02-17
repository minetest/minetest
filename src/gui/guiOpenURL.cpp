/*
Part of Minetest
Copyright (C) 2023 rubenwardy

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

#include "guiOpenURL.h"
#include "guiButton.h"
#include "guiEditBoxWithScrollbar.h"
#include <IGUIEditBox.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>

#ifdef HAVE_TOUCHSCREENGUI
	#include "client/renderingengine.h"
#endif

#include "porting.h"
#include "gettext.h"
#ifdef USE_CURL
#include <curl/urlapi.h>
#endif

const int ID_url = 256;
const int ID_open = 259;
const int ID_cancel = 261;

GUIOpenURLMenu::GUIOpenURLMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		ISimpleTextureSource *tsrc, const std::string &url
):
	GUIModalMenu(env, parent, id, menumgr),
	m_tsrc(tsrc),
	url(url)
{
}

std::string colorize_url(const std::string &url) {
#ifdef USE_CURL
	// Forbid escape codes in URL
	if (url.find('\x1b') != std::string::npos) {
		return "";
	}

	CURLU *h = curl_url();
	auto rc = curl_url_set(h, CURLUPART_URL, url.c_str(), 0);
	if (rc != CURLUE_OK) {
		curl_url_cleanup(h);
		return "";
	}

	char *fragment;
	char *host;
	char *password;
	char *path;
	char *port;
	char *query;
	char *scheme;
	char *user;
	char *zoneid;
	curl_url_get(h, CURLUPART_FRAGMENT, &fragment, 0);
	// Get host as punycode to explicitly show homographs
	curl_url_get(h, CURLUPART_HOST, &host, CURLU_PUNYCODE);
	curl_url_get(h, CURLUPART_PASSWORD, &password, 0);
	curl_url_get(h, CURLUPART_PATH, &path, 0);
	curl_url_get(h, CURLUPART_PORT, &port, 0);
	curl_url_get(h, CURLUPART_QUERY, &query, 0);
	curl_url_get(h, CURLUPART_SCHEME, &scheme, 0);
	curl_url_get(h, CURLUPART_USER, &user, 0);
	curl_url_get(h, CURLUPART_ZONEID, &zoneid, 0);

	std::ostringstream os;

	const std::string white = "\x1b(c@#fff)";
	const std::string grey = "\x1b(c@#aaa)";

	os << grey << scheme << "://";
	if (user != NULL)
		os << user;
	if (password != NULL)
		os << ":" << password;
	if (user != NULL || password != NULL)
		os << "@";

	os << white << host << grey;
	if (port != NULL)
		os << ":" << port;
	os << path;
	if (query != NULL)
		os << "?" << query;
	if (fragment != NULL)
		os << "#" << fragment;

	curl_url_cleanup(h);
	return os.str();
#else
	return str;
#endif
}

void GUIOpenURLMenu::regenerateGui(v2u32 screensize)
{
	/*
		Remove stuff
	*/
	removeAllChildren();

	/*
		Calculate new sizes and positions
	*/
#ifdef HAVE_TOUCHSCREENGUI
	const float s = m_gui_scale * RenderingEngine::getDisplayDensity() / 2;
#else
	const float s = m_gui_scale;
#endif
	DesiredRect = core::rect<s32>(
		screensize.X / 2 - 580 * s / 2,
		screensize.Y / 2 - 250 * s / 2,
		screensize.X / 2 + 580 * s / 2,
		screensize.Y / 2 + 250 * s / 2
	);
	recalculateAbsolutePosition(false);

	v2s32 size = DesiredRect.getSize();
	v2s32 topleft_client(40 * s, 0);

	/*
		Add stuff
	*/
	s32 ypos = 40 * s;

	{
		core::rect<s32> rect(0, 0, 500 * s, 20 * s);
		rect += topleft_client + v2s32(20 * s, ypos);
		gui::StaticText::add(Environment, wstrgettext("Open URL?"), rect,
				false, true, this, -1);
	}

	ypos += 50 * s;

	{
		core::rect<s32> rect(0, 0, 440 * s, 60 * s);

		auto mono_font = g_fontengine->getFont(FONT_SIZE_UNSPECIFIED, FM_Mono);
		int scrollbar_width = Environment->getSkin()->getSize(gui::EGDS_SCROLLBAR_SIZE);
		int max_cols = (rect.getWidth() - scrollbar_width - 10) / mono_font->getDimension(L"x").Width;
		errorstream << max_cols << std::endl;

		std::string text = colorize_url(url);
		if (text.empty()) {
			quitMenu();
			return;
		}

		text = wrap_rows(text, max_cols, true);

		rect += topleft_client + v2s32(20 * s, ypos);
		IGUIEditBox *e = new GUIEditBoxWithScrollBar(utf8_to_wide(text).c_str(), true, Environment,
				this, ID_url, rect, m_tsrc, false, true);
		e->setMultiLine(true);
		e->setWordWrap(true);
		e->setTextAlignment(gui::EGUIA_UPPERLEFT, gui::EGUIA_UPPERLEFT);
		e->setDrawBorder(true);
		e->setDrawBackground(true);

		e->setOverrideFont(mono_font);
		e->drop();
	}

	ypos += 80 * s;
	{
		core::rect<s32> rect(0, 0, 100 * s, 40 * s);
		rect = rect + v2s32(size.X / 2 - 150 * s, ypos);
		GUIButton::addButton(Environment, rect, m_tsrc, this, ID_open,
				wstrgettext("Open").c_str());
	}
	{
		core::rect<s32> rect(0, 0, 100 * s, 40 * s);
		rect = rect + v2s32(size.X / 2 + 50 * s, ypos);
		GUIButton::addButton(Environment, rect, m_tsrc, this, ID_cancel,
				wstrgettext("Cancel").c_str());
	}
}

void GUIOpenURLMenu::drawMenu()
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

bool GUIOpenURLMenu::OnEvent(const SEvent &event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		if ((event.KeyInput.Key == KEY_ESCAPE ||
				event.KeyInput.Key == KEY_CANCEL) &&
				event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}
		if (event.KeyInput.Key == KEY_RETURN && event.KeyInput.PressedDown) {
			porting::open_url(url);
			quitMenu();
			return true;
		}
	}

	if (event.EventType == EET_GUI_EVENT) {
		if (event.GUIEvent.EventType == gui::EGET_ELEMENT_FOCUS_LOST &&
				isVisible()) {
			if (!canTakeFocus(event.GUIEvent.Element)) {
				infostream << "GUIOpenURLMenu: Not allowing focus change."
					<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}

		if (event.GUIEvent.EventType == gui::EGET_BUTTON_CLICKED) {
			switch (event.GUIEvent.Caller->getID()) {
			case ID_open:
				porting::open_url(url);
				quitMenu();
				return true;
			case ID_cancel:
				quitMenu();
				return true;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

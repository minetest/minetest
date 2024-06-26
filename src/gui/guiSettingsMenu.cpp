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

#include <sstream>
#include "guiSettingsMenu.h"
#include "debug.h"
#include "guiButton.h"
#include "guiScrollBar.h"
#include "guiFormSpecMenu.h"
#include "client/inputhandler.h"
#include "client/client.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "settings.h"

#include "gettext.h"

const int ID_soundText = 263;
const int ID_soundExitButton = 264;
const int ID_soundSlider = 265;
const int ID_soundMuteButton = 266;

struct GuiFormspecHandler : public TextDest
{
	GuiFormspecHandler()
	{
		//m_formname = formname;
	}

	void gotText(const StringMap &fields)
	{
		if (m_formname == "MT_PAUSE_MENU") {
			if (fields.find("btn_sound") != fields.end()) {
//				g_gamecallback->changeVolume();
				return;
			}

			return;
		}
	}
};


GUISettingsMenu::GUISettingsMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr, ISimpleTextureSource *tsrc, InputHandler *input,
		RenderingEngine *engine, Client *client, ISoundManager* sound_manager
):
	GUIModalMenu(env, parent, id, menumgr),
	m_filtered_pages(),
	m_tsrc(tsrc),
	m_curr_page(nullptr),
	m_game_ui(new GameUI()),
	m_fs_handler(new GuiFormspecHandler()),
	m_input(input),
	m_rendering_engine(engine),
	m_client(client),
	m_sound_manager(sound_manager)
{
	addPage({
		"accessibility",
		"Accessibility",
		std::string(),
		{
			{"language"},
			{"General", true},
			{"font_size"},
			{"chat_font_size"},
			{"gui_scaling"}
		}
	});
	m_curr_page = &m_filtered_pages[0];
	addPage({
		"things",
		"Things",
		"Junk food",
		{
			{"language"},
			{"General", true},
			{"font_size"},
			{"chat_font_size"},
			{"gui_scaling"}
		}
	});
	addPage({
		"help",
		"Help",
		"Junk food",
		{
			{"language"},
			{"General", true},
			{"font_size"},
			{"chat_font_size"},
			{"gui_scaling"}
		}
	});
}

std::string GUISettingsMenu::build_page_components(SettingsPage *curr_page)
{
	if (!curr_page) curr_page = m_curr_page;
	
	std::string result;
	std::string last_heading;
	for (auto& item : curr_page->content)
	{
		if (item.is_heading)
			last_heading = item.heading;
		else {
			
		}
	}
	
	return result;
}

static std::string make_scrollbaroptions_for_scroll_container(
	double visible_l,
	double total_l, 
	double scroll_factor)
{
	const double max = total_l - visible_l;
	const double thumb_size = (visible_l / total_l) * max; 
	return "scrollbaroptions[min=0;max=" + std::to_string(max / scroll_factor)
		+ ";thumbsize=" + std::to_string(thumb_size / scroll_factor) + "]";
}

void GUISettingsMenu::regenerateGui(v2u32 screensize)
{
	removeAllChildren();
	
	std::ostringstream os;
	
	bool enable_touch = false;
	int extra_h = 1;
	double tabsize_w = enable_touch ? 16.5 : 15.5;
	double tabsize_h = enable_touch ? (10 - extra_h) : 12;

	double scrollbar_w = enable_touch ? 0.6 : 0.4;
	double left_pane_width = enable_touch ? 4.5 : 4.25;
	double left_pane_padding = 0.25;
	double search_width = left_pane_width + scrollbar_w - (0.75 * 2);
	double back_w = 3;
	double checkbox_w = (tabsize_w - back_w - 2*0.2) / 2;
	
	os << "formspec_version[6]";
	os << "size[" << tabsize_w << ',' << tabsize_h + extra_h << ']';
	if (enable_touch) os << "padding[0.01,0.01]";
	os << "bgcolor[#0000]";
	// put hack here?
	os << "box[0,0;" << tabsize_w << ',' << tabsize_h << ";#0000008C]";
	os << "button[0," << tabsize_h + 0.2 << ';' << back_w << ",0.8;back;" << "Back" << ']';
	
	os << "box[" << back_w + 0.2 << ',' << tabsize_h + 0.2 << ';' << checkbox_w << ",0.8;#0000008C]";
	os << "checkbox[" << back_w + 2*0.2 << "," << tabsize_h + 0.6 << ";show_technical_names;"
		<< "Show technical names" << ';' << "false" << ']';
		
	os << "box[" << back_w + 2*0.2 + checkbox_w << ',' << tabsize_h + 0.2 << ';'
		<< checkbox_w << ",0.8;#0000008C]";
	os << "checkbox[" << back_w + 3*0.2 + checkbox_w << ',' << tabsize_h + 0.6 << ";show_advanced;"
		<< "Show advanced settings" << ';' << "true" << ']';
	// Searchbox
	os << "field[0.25,0.25;" << search_width << ",0.75;search_query;;" << "query" << ']';
	os << "field_enter_after_edit[search_query;true]";
	
	os << "container[" << search_width << ",0.25]";
	{
		os << "image_button[0,0;0.75,0.75;search.png;search;]";
		os << "image_button[0.75,0;0.75,0.75;clear.png;search_clear;]";
		os << "tooltip[search;Search]";
		os << "tooltip[search_clear;Clear]";
	}
	os << "container_end[]";
	os << "scroll_container[0.25,1.25;" << left_pane_width << ',' << tabsize_h - 1.5 << ";leftscroll;vertical;0.1]";
	os << "style_type[button;border=false;bgcolor=#3333]";
	os << "style_type[button:hover;border=false;bgcolor=#6663]";
	
	double y = 0;
	std::string last_section;
	for (auto& page : m_filtered_pages)
	{
		if (page.section != last_section)
		{
			os << "label[0.1," << y + 0.41 << ';' << page.section << ']';
			last_section = page.section;
			y += 0.82;
		}
		os << "box[0," << y << ';' << left_pane_width-left_pane_padding << ",0.8;" << "#3339]";
		os << "button[0," << y << ';' << left_pane_width-left_pane_padding << ",0.8;page_"
			<< page.id << ';' << page.title << ']';
		y += 0.82;
	}
	// TODO show "No results"
	
	os << "scroll_container_end[]";
	
	if (y >= tabsize_h - 1.25)
	{
		os << make_scrollbaroptions_for_scroll_container(tabsize_h - 1.5, y, 0.1);
		os << "scrollbar[" << left_pane_width + 0.25 << ",1.25;" << scrollbar_w << ',' << tabsize_h - 1.5
			<< ";vertical;leftscroll;" << 0 << ']';
	}
	
	os << "style_type[button;border=;bgcolor=]";
	
	// TODO build components
	os << build_page_components(nullptr);
	
	double right_pane_width = tabsize_w - left_pane_width - 0.375 - 2*scrollbar_w - 0.25;
	os << "scroll_container[" << tabsize_w - right_pane_width - scrollbar_w << ",0;"
		<< right_pane_width << ',' << tabsize_h << ";rightscroll;vertical;0.1]";
	
	y = 0.25;
	// Loop through components
	
	os << "scroll_container_end[]";
	
	if (y >= tabsize_h)
	{
		os << make_scrollbaroptions_for_scroll_container(tabsize_h, y + 0.375, 0.1);
		os << "scrollbar[" << tabsize_w - scrollbar_w << ",0;" << scrollbar_w << ','
			<< tabsize_h << ";vertical;rightscroll;" << 0 << "]";
	}
	
	FormspecFormSource *fs_src = new FormspecFormSource(os.str());
	GuiFormspecHandler *txt_dst = new GuiFormspecHandler();

	auto *&formspec = m_game_ui->getFormspecGUI();
	GUIFormSpecMenu::create(formspec, m_client, m_rendering_engine->get_gui_env(),
	 		&m_input->joystick, fs_src, txt_dst, m_client->getFormspecPrepend(),
	 		m_sound_manager);

}

void GUISettingsMenu::addPage(const SettingsPage& page)
{
	m_filtered_pages.push_back(page);
}


void GUISettingsMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	//video::SColor bgcolor(0, 0, 0, 0);
	//driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);
	//gui::IGUIElement::draw();
}

bool GUISettingsMenu::OnEvent(const SEvent& event)
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
				infostream << "GUISettingsMenu: Not allowing focus change."
				<< std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if (event.GUIEvent.EventType == gui::EGET_SCROLL_BAR_CHANGED) {
			if (event.GUIEvent.Caller->getID() == ID_soundSlider) {
				s32 pos = static_cast<GUIScrollBar *>(event.GUIEvent.Caller)->getPos();
				g_settings->setFloat("sound_volume", (float) pos / 100);

				gui::IGUIElement *e = getElementFromId(ID_soundText);
				e->setText(fwgettext("Sound Volume: %d%%", pos).c_str());
				return true;
			}
		}

	}

	return Parent ? Parent->OnEvent(event) : false;
}

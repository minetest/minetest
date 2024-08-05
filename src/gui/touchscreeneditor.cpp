/*
Minetest
Copyright (C) 2024 grorp, Gregor Parzefall <grorp@posteo.de>

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

#include "touchscreeneditor.h"
#include "touchscreenlayout.h"
#include "touchcontrols.h"
#include "irr_gui_ptr.h"
#include "gettext.h"
#include "client/renderingengine.h"
#include "settings.h"

#include "IGUIButton.h"
#include "IGUIStaticText.h"
#include "IGUIFont.h"

GUITouchscreenLayout::GUITouchscreenLayout(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr, ISimpleTextureSource *tsrc
):
	GUIModalMenu(env, parent, id, menumgr),
	m_tsrc(tsrc)
{
	if (g_touchcontrols)
		m_layout = g_touchcontrols->getLayout();
	else
		m_layout = ButtonLayout::loadFromSettings(
				Environment->getVideoDriver()->getScreenSize());

	m_gui_help_text = grab_gui_element<IGUIStaticText>(Environment->addStaticText(
			L"", core::recti(), false, false, this, -1));
	m_gui_help_text->setTextAlignment(EGUIA_CENTER, EGUIA_CENTER);

	m_gui_add_btn = grab_gui_element<IGUIButton>(Environment->addButton(
			core::recti(), this, -1, wstrgettext("Add button").c_str()));
	m_gui_reset_btn = grab_gui_element<IGUIButton>(Environment->addButton(
			core::recti(), this, -1, wstrgettext("Reset").c_str()));
	m_gui_done_btn = grab_gui_element<IGUIButton>(Environment->addButton(
			core::recti(), this, -1, wstrgettext("Done").c_str()));

	m_gui_remove_btn = grab_gui_element<IGUIButton>(Environment->addButton(
		core::recti(), this, -1, wstrgettext("Remove").c_str()));
}

GUITouchscreenLayout::~GUITouchscreenLayout()
{
	ButtonLayout::clearTextureCache();
}

void GUITouchscreenLayout::regenerateGui(v2u32 screensize)
{
	DesiredRect = core::recti(0, 0, screensize.X, screensize.Y);
	recalculateAbsolutePosition(false);

	if (m_add_mode)
		regenerateGUIImagesAddMode(screensize);
	else
		regenerateGUIImagesRegular(screensize);
	regenerateMenu(screensize);
}

void GUITouchscreenLayout::clearGUIImages()
{
	m_gui_images.clear();
	m_gui_images_target_pos.clear();

	m_add_layout.layout.clear();
	m_add_button_titles.clear();
}

void GUITouchscreenLayout::regenerateGUIImagesRegular(v2u32 screensize)
{
	auto old_gui_images = m_gui_images;
	clearGUIImages();

	for (const auto &[btn, meta] : m_layout.layout) {
		core::recti rect = ButtonLayout::getRect(btn, meta, m_tsrc);
		std::shared_ptr<IGUIImage> img;

		if (old_gui_images.count(btn) > 0) {
			img = old_gui_images.at(btn);
			// Update size, keep position. Position is interpolated in interpolateGUIImages.
			img->setRelativePosition(core::recti(
					img->getRelativePosition().UpperLeftCorner, rect.getSize()));
		} else {
			img = grab_gui_element<IGUIImage>(Environment->addImage(rect, this, -1));
			img->setImage(ButtonLayout::getTexture(btn, m_tsrc));
			img->setScaleImage(true);
		}

		m_gui_images[btn] = img;
		m_gui_images_target_pos[btn] = rect.UpperLeftCorner;
	}
}

void GUITouchscreenLayout::regenerateGUIImagesAddMode(v2u32 screensize)
{
	clearGUIImages();

	auto missing_buttons = m_layout.getMissingButtons();

	layout_button_grid(screensize, m_tsrc, missing_buttons, [&] (touch_gui_button_id btn, v2s32 pos, core::recti rect) {
		auto img = grab_gui_element<IGUIImage>(Environment->addImage(rect, this, -1));
		img->setImage(ButtonLayout::getTexture(btn, m_tsrc));
		img->setScaleImage(true);

		m_gui_images[btn] = img;
		m_gui_images_target_pos[btn] = rect.UpperLeftCorner;

		m_add_layout.layout[btn] = ButtonMeta{pos, (u32)rect.getHeight()};

		IGUIStaticText *text = Environment->addStaticText(L"", core::recti(),
				false, false,this, -1);
		make_button_grid_title(text, btn, pos, rect);
		m_add_button_titles.push_back(grab_gui_element<IGUIStaticText>(text));
	});
}

void GUITouchscreenLayout::interpolateGUIImages()
{
	if (m_add_mode)
		return;

	for (auto &[btn, gui_image] : m_gui_images) {
		bool interpolate = !m_dragging || m_selected_btn != btn;

		v2s32 cur_pos_int = gui_image->getRelativePosition().UpperLeftCorner;
		v2s32 tgt_pos_int = m_gui_images_target_pos.at(btn);
		v2f cur_pos(cur_pos_int.X, cur_pos_int.Y);
		v2f tgt_pos(tgt_pos_int.X, tgt_pos_int.Y);

		if (interpolate && cur_pos.getDistanceFrom(tgt_pos) > 2.0f) {
			v2f pos = cur_pos.getInterpolated(tgt_pos, 0.5f);
			gui_image->setRelativePosition(v2s32(core::round32(pos.X), core::round32(pos.Y)));
		} else {
			gui_image->setRelativePosition(tgt_pos_int);
		}
	}
}

static void layout_menu_row(v2u32 screensize,
		const std::vector<std::shared_ptr<IGUIButton>> &row,
		const std::vector<std::shared_ptr<IGUIButton>> &full_row, bool bottom)
{
	s32 spacing = RenderingEngine::getDisplayDensity() * 4.0f;

	s32 btn_w = 0;
	s32 btn_h = 0;
	for (const auto &btn : full_row) {
		IGUIFont *font = btn->getActiveFont();
		core::dimension2du dim = font->getDimension(btn->getText());
		btn_w = std::max(btn_w, (s32)(dim.Width * 1.5f));
		btn_h = std::max(btn_h, (s32)(dim.Height * 2.5f));
	}

	const s32 row_width = ((btn_w + spacing) * row.size()) - spacing;
	s32 x = screensize.X / 2 - row_width / 2;
	const s32 y = bottom ? screensize.Y - spacing - btn_h : spacing;

	for (const auto &btn : row) {
		btn->setRelativePosition(core::recti(
				v2s32(x, y), core::dimension2du(btn_w, btn_h)));
		x += btn_w + spacing;
	}
}

void GUITouchscreenLayout::regenerateMenu(v2u32 screensize)
{
	// Discard invalid selection. May happen when...
	// 1. a button is removed.
	// 2. adding a button fails and it disappears from the layout again.
	if (m_selected_btn != touch_gui_button_id_END &&
			m_layout.layout.count(m_selected_btn) == 0)
		m_selected_btn = touch_gui_button_id_END;

	bool have_selection = m_selected_btn != touch_gui_button_id_END;

	if (m_add_mode)
		m_gui_help_text->setText(wstrgettext("Start dragging a button to add. Tap outside to cancel.").c_str());
	else if (!have_selection)
		m_gui_help_text->setText(wstrgettext("Tap a button to select it. Drag a button to move it.").c_str());
	else
		m_gui_help_text->setText(wstrgettext("Tap outside to deselect.").c_str());

	IGUIFont *font = m_gui_help_text->getActiveFont();
	core::dimension2du dim = font->getDimension(m_gui_help_text->getText());
	s32 height = dim.Height * 2.5f;
	s32 pos_y = (m_add_mode || have_selection) ? 0 : screensize.Y - height;
	m_gui_help_text->setRelativePosition(core::recti(
			v2s32(0, pos_y),
			core::dimension2du(screensize.X, height)));

	bool no_selection_visible = !m_add_mode && !have_selection;
	bool add_visible = no_selection_visible && !m_layout.getMissingButtons().empty();

	m_gui_add_btn->setVisible(add_visible);
	m_gui_reset_btn->setVisible(no_selection_visible);
	m_gui_done_btn->setVisible(no_selection_visible);

	if (no_selection_visible) {
		std::vector row1{m_gui_add_btn, m_gui_reset_btn, m_gui_done_btn};
		if (add_visible) {
			layout_menu_row(screensize, row1, row1, false);
		} else {
			std::vector row1_reduced{m_gui_reset_btn, m_gui_done_btn};
			layout_menu_row(screensize, row1_reduced, row1, false);
		}
	}

	bool remove_visible = !m_add_mode && have_selection &&
			!ButtonLayout::isButtonRequired(m_selected_btn);

	m_gui_remove_btn->setVisible(remove_visible);

	if (remove_visible) {
		std::vector row2{m_gui_remove_btn};
		layout_menu_row(screensize, row2, row2, true);
	}
}

void GUITouchscreenLayout::drawMenu()
{
	video::IVideoDriver *driver = Environment->getVideoDriver();

	video::SColor bgcolor(140, 0, 0, 0);
	video::SColor selection_color(255, 128, 128, 128);
	video::SColor error_color(255, 255, 0, 0);

	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	// Done here instead of in OnPostRender to avoid drag&drop lagging behind
	// input by one frame.
	// Must be done before drawing selection rectangle.
	interpolateGUIImages();

	bool draw_selection = m_gui_images.count(m_selected_btn) > 0;
	if (draw_selection)
		driver->draw2DRectangle(selection_color,
				m_gui_images.at(m_selected_btn)->getAbsolutePosition(),
				&AbsoluteClippingRect);

	if (m_dragging) {
		for (const auto &rect : m_error_rects)
			driver->draw2DRectangle(error_color, rect, &AbsoluteClippingRect);
	}

	IGUIElement::draw();
}

void GUITouchscreenLayout::updateDragState(v2u32 screensize, v2s32 mouse_movement)
{
	ButtonMeta &meta = m_layout.layout.at(m_selected_btn);
	meta.pos += mouse_movement;

	core::recti rect = ButtonLayout::getRect(m_selected_btn, meta, m_tsrc);
	rect.constrainTo(core::recti(v2s32(0, 0), core::dimension2du(screensize)));
	meta.pos = rect.getCenter();

	rect = ButtonLayout::getRect(m_selected_btn, meta, m_tsrc);

	m_error_rects.clear();
	for (const auto &[other_btn, other_meta] : m_layout.layout) {
		if (other_btn == m_selected_btn)
			continue;
		core::recti other_rect = ButtonLayout::getRect(other_btn, other_meta, m_tsrc);
		if (other_rect.isRectCollided(rect))
			m_error_rects.push_back(other_rect);
	}
	if (m_error_rects.empty())
		m_last_good_layout = m_layout;
}

bool GUITouchscreenLayout::OnEvent(const SEvent& event)
{
	if (event.EventType == EET_KEY_INPUT_EVENT) {
		if (event.KeyInput.Key == KEY_ESCAPE && event.KeyInput.PressedDown) {
			quitMenu();
			return true;
		}
	}

	core::dimension2du screensize = Environment->getVideoDriver()->getScreenSize();

	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		v2s32 mouse_pos = v2s32(event.MouseInput.X, event.MouseInput.Y);

		switch (event.MouseInput.Event) {
		case EMIE_LMOUSE_PRESSED_DOWN: {
			if (m_mouse_down)
				return true; // This actually fixes a bug.
			m_mouse_down = true;
			m_last_mouse_pos = mouse_pos;

			IGUIElement *el = Environment->getRootGUIElement()->getElementFromPoint(mouse_pos);
			// Clicking on nothing deselects.
			m_selected_btn = touch_gui_button_id_END;
			for (const auto &[btn, gui_image] : m_gui_images) {
				if (el == gui_image.get()) {
					m_selected_btn = btn;
					break;
				}
			}

			if (m_add_mode) {
				if (m_selected_btn != touch_gui_button_id_END) {
					m_dragging = true;
					m_last_good_layout = m_layout;
					m_layout.layout[m_selected_btn] = m_add_layout.layout.at(m_selected_btn);
					updateDragState(screensize, v2s32(0, 0));
				}

				// Clicking on nothing quits add mode without adding a button.
				m_add_mode = false;
			}

			regenerateGui(screensize);
			return true;
		}
		case EMIE_MOUSE_MOVED: {
			if (m_mouse_down && m_selected_btn != touch_gui_button_id_END) {
				if (!m_dragging) {
					m_dragging = true;
					m_last_good_layout = m_layout;
				}
				updateDragState(screensize, mouse_pos - m_last_mouse_pos);

				regenerateGui(screensize);
			}

			m_last_mouse_pos = mouse_pos;
			return true;
		}
		case EMIE_LMOUSE_LEFT_UP: {
			m_mouse_down = false;

			if (m_dragging) {
				m_dragging = false;
				if (!m_error_rects.empty())
					m_layout = m_last_good_layout;

				regenerateGui(screensize);
			}

			return true;
		}
		default:
			break;
		}
	}

	if (event.EventType == EET_GUI_EVENT) {
		switch (event.GUIEvent.EventType) {
		case EGET_BUTTON_CLICKED: {
			if (event.GUIEvent.Caller == m_gui_add_btn.get()) {
				m_add_mode = true;
				regenerateGui(screensize);
				return true;
			}

			if (event.GUIEvent.Caller == m_gui_reset_btn.get()) {
				m_layout = ButtonLayout::getDefault(screensize);
				regenerateGui(screensize);
				return true;
			}

			if (event.GUIEvent.Caller == m_gui_done_btn.get()) {
				if (g_touchcontrols)
					g_touchcontrols->applyLayout(m_layout);
				std::ostringstream oss;
				m_layout.serializeJson(oss);
				g_settings->set("touch_layout", oss.str());
				quitMenu();
				return true;
			}

			if (event.GUIEvent.Caller == m_gui_remove_btn.get()) {
				m_layout.layout.erase(m_selected_btn);
				regenerateGui(screensize);
				return true;
			}

			break;
		}
		default:
			break;
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

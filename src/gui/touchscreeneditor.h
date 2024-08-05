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

#pragma once

#include "touchscreenlayout.h"
#include "modalMenu.h"

#include "IGUIImage.h"

#include <memory>
#include <unordered_map>

class ISimpleTextureSource;

class GUITouchscreenLayout : public GUIModalMenu
{
public:
	GUITouchscreenLayout(gui::IGUIEnvironment* env,
            gui::IGUIElement* parent, s32 id,
            IMenuManager *menumgr, ISimpleTextureSource *tsrc);
	~GUITouchscreenLayout();

	void regenerateGui(v2u32 screensize);
	void drawMenu();
	bool OnEvent(const SEvent& event);

protected:
	std::wstring getLabelByID(s32 id) { return L""; }
	std::string getNameByID(s32 id) { return ""; }

private:
	ISimpleTextureSource *m_tsrc;

	ButtonLayout m_layout;
	v2u32 m_last_screensize;
	s32 m_button_size;

	std::unordered_map<touch_gui_button_id, std::shared_ptr<gui::IGUIImage>> m_gui_images;
	// unused if m_add_mode == true
	std::unordered_map<touch_gui_button_id, v2s32> m_gui_images_target_pos;
	void clearGUIImages();
	void regenerateGUIImagesRegular(v2u32 screensize);
	void regenerateGUIImagesAddMode(v2u32 screensize);
	void interpolateGUIImages();

	bool m_mouse_down = false;
	v2s32 m_last_mouse_pos;
	touch_gui_button_id m_selected_btn = touch_gui_button_id_END;

	bool m_dragging = false;
	ButtonLayout m_last_good_layout;
	std::vector<core::recti> m_error_rects;
	void updateDragState(v2u32 screensize, v2s32 mouse_movement);

	bool m_add_mode = false;
	ButtonLayout m_add_layout;
	std::vector<std::shared_ptr<gui::IGUIStaticText>> m_add_button_titles;

	std::shared_ptr<gui::IGUIStaticText> m_gui_help_text;

	std::shared_ptr<gui::IGUIButton> m_gui_add_btn;
	std::shared_ptr<gui::IGUIButton> m_gui_reset_btn;
	std::shared_ptr<gui::IGUIButton> m_gui_done_btn;

	std::shared_ptr<gui::IGUIButton> m_gui_remove_btn;

	void regenerateMenu(v2u32 screensize);
};

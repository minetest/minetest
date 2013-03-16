/*
Copyright (C) eh, you figure it out

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

#include "guiSettingsMenu.h"
#include "debug.h"
#include "serialization.h"
#include <string>
#include <IGUICheckBox.h>
#include <IGUIButton.h>
#include <IGUIScrollBar.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "main.h"
// For IGameCallback
#include "guiPauseMenu.h"
// For MainMenuData
#include "guiMainMenu.h"
#include "gettext.h"

enum
{
	GUI_ID_QUIT_BUTTON = 101,
	GUI_ID_NAME_INPUT,
	GUI_ID_ADDRESS_INPUT,
	GUI_ID_PORT_INPUT,
	GUI_ID_FANCYTREE_CB,
	GUI_ID_SMOOTH_LIGHTING_CB,
	GUI_ID_3D_CLOUDS_CB,
	GUI_ID_OPAQUE_WATER_CB,
	GUI_ID_MIPMAP_CB,
	GUI_ID_ANISOTROPIC_CB,
	GUI_ID_BILINEAR_CB,
	GUI_ID_TRILINEAR_CB,
	GUI_ID_SHADERS_CB,
	GUI_ID_PRELOAD_ITEM_VISUALS_CB,
	GUI_ID_ENABLE_PARTICLES_CB,
	GUI_ID_DAMAGE_CB,
	GUI_ID_CREATIVE_CB,
	GUI_ID_PUBLIC_CB,
	GUI_ID_JOIN_GAME_BUTTON,
	GUI_ID_CHANGE_VOLUME_BUTTON,
	GUI_ID_CHANGE_KEYS_BUTTON,
	GUI_ID_DELETE_WORLD_BUTTON,
	GUI_ID_CREATE_WORLD_BUTTON,
	GUI_ID_CONFIGURE_WORLD_BUTTON,
	GUI_ID_WORLD_LISTBOX,
	GUI_ID_TAB_CONTROL,
	GUI_ID_SERVERLIST,
	GUI_ID_SERVERLIST_TOGGLE,
	GUI_ID_SERVERLIST_DELETE,
};

GUISettingsMenu::GUISettingsMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IGameCallback *gamecallback,
		IMenuManager *menumgr
):
	GUIModalMenu(env, parent, id, menumgr),
	m_gamecallback(gamecallback)
{
}

GUISettingsMenu::~GUISettingsMenu()
{
	removeChildren();
}

void GUISettingsMenu::removeChildren()
{
	{
		gui::IGUIElement *e = getElementFromId(267);
		if(e != NULL)
			e->remove();
	}
}

void GUISettingsMenu::regenerateGui(v2u32 screensize)
{
	/*
		Remove stuff
	*/
	removeChildren();
	
	/*
		Calculate new sizes and positions
	*/
	
	v2s32 size(screensize.X, screensize.Y);
	
	core::rect<s32> rect(
			screensize.X/2 - size.X/2,
			screensize.Y/2 - size.Y/2,
			screensize.X/2 + size.X/2,
			screensize.Y/2 + size.Y/2
	);
	
	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	//v2s32 size = rect.getSize();
	
	/*
		Add stuff
	*/
	
	changeCtype("");
	
	//v2s32 center(size.X/2, size.Y/2);
	v2s32 c800(size.X/2-400, size.Y/2-270);
	
	m_topleft_client = c800 + v2s32(90, 70+50+30);
	m_size_client = v2s32(620, 270);

	m_size_server = v2s32(620, 140);
	
	
	m_topleft_server = m_topleft_client + v2s32(0, m_size_client.Y+20);
	
	{
		core::rect<s32> rect(0, 0, 10, m_size_client.Y);
		rect += m_topleft_client + v2s32(15, 0);
		const wchar_t *text = L"S\nE\nT\nT\nI\nN\nG\nS";
		gui::IGUIStaticText *t =
		Environment->addStaticText(text, rect, false, true, this, -1);
		t->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
	}
	
	s32 option_x = 70;
	s32 option_y = 50;
	u32 option_w = 150;

	MainMenuData *m_data;
	{
		core::rect<s32> rect(0, 0, option_w, 30);
		rect += m_topleft_client + v2s32(option_x, option_y);
		Environment->addCheckBox(m_data->fancy_trees, rect, this,
				GUI_ID_FANCYTREE_CB, wgettext("Fancy trees"));
	}
	{
		core::rect<s32> rect(0, 0, option_w, 30);
		rect += m_topleft_client + v2s32(option_x, option_y+20);
		Environment->addCheckBox(m_data->smooth_lighting, rect, this,
				GUI_ID_SMOOTH_LIGHTING_CB, wgettext("Smooth Lighting"));
	}
	{
		core::rect<s32> rect(0, 0, option_w, 30);
		rect += m_topleft_client + v2s32(option_x, option_y+20*2);
		Environment->addCheckBox(m_data->clouds_3d, rect, this,
				GUI_ID_3D_CLOUDS_CB, wgettext("3D Clouds"));
	}
	{
		core::rect<s32> rect(0, 0, option_w, 30);
		rect += m_topleft_client + v2s32(option_x, option_y+20*3);
		Environment->addCheckBox(m_data->opaque_water, rect, this,
				GUI_ID_OPAQUE_WATER_CB, wgettext("Opaque water"));
	}


	// Anisotropic/mipmap/bi-/trilinear settings

	{
		core::rect<s32> rect(0, 0, option_w+20, 30);
		rect += m_topleft_client + v2s32(option_x+175, option_y);
		Environment->addCheckBox(m_data->mip_map, rect, this,
				   GUI_ID_MIPMAP_CB, wgettext("Mip-Mapping"));
	}

	{
		core::rect<s32> rect(0, 0, option_w+20, 30);
		rect += m_topleft_client + v2s32(option_x+175, option_y+20);
		Environment->addCheckBox(m_data->anisotropic_filter, rect, this,
				   GUI_ID_ANISOTROPIC_CB, wgettext("Anisotropic Filtering"));
	}

	{
		core::rect<s32> rect(0, 0, option_w+20, 30);
		rect += m_topleft_client + v2s32(option_x+175, option_y+20*2);
		Environment->addCheckBox(m_data->bilinear_filter, rect, this,
				   GUI_ID_BILINEAR_CB, wgettext("Bi-Linear Filtering"));
	}

	{
		core::rect<s32> rect(0, 0, option_w+20, 30);
		rect += m_topleft_client + v2s32(option_x+175, option_y+20*3);
		Environment->addCheckBox(m_data->trilinear_filter, rect, this,
				   GUI_ID_TRILINEAR_CB, wgettext("Tri-Linear Filtering"));
	}

	// shader/on demand image loading/particles settings
	{
		core::rect<s32> rect(0, 0, option_w+20, 30);
		rect += m_topleft_client + v2s32(option_x+175*2, option_y);
		Environment->addCheckBox(m_data->enable_shaders, rect, this,
				GUI_ID_SHADERS_CB, wgettext("Shaders"));
	}

	{
		core::rect<s32> rect(0, 0, option_w+20+20, 30);
		rect += m_topleft_client + v2s32(option_x+175*2, option_y+20);
		Environment->addCheckBox(m_data->preload_item_visuals, rect, this,
				GUI_ID_PRELOAD_ITEM_VISUALS_CB, wgettext("Preload item visuals"));
	}

	{
		core::rect<s32> rect(0, 0, option_w+20+20, 30);
		rect += m_topleft_client + v2s32(option_x+175*2, option_y+20*2);
		Environment->addCheckBox(m_data->enable_particles, rect, this,
				GUI_ID_ENABLE_PARTICLES_CB, wgettext("Enable Particles"));
	}

	// Volume change button
	{
		core::rect<s32> rect(0, 0, 120, 30);
		rect += m_topleft_client + v2s32(option_x, option_y+120);
		Environment->addButton(rect, this,
				GUI_ID_CHANGE_VOLUME_BUTTON, wgettext("Sound Volume"));
	}
	// Key change button
	{
		core::rect<s32> rect(0, 0, 120, 30);
		/*rect += m_topleft_client + v2s32(m_size_client.X-120-30,
				m_size_client.Y-30-20);*/
		rect += m_topleft_client + v2s32(option_x+120+20, option_y+120);
		Environment->addButton(rect, this,
				GUI_ID_CHANGE_KEYS_BUTTON, wgettext("Change keys"));
	}
	changeCtype("C");
}

void GUISettingsMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	video::SColor bgcolor(140,0,0,0);
	//driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);
	core::rect<s32> rect(0, 0, m_size_client.X, m_size_client.Y);
	rect += AbsoluteRect.UpperLeftCorner + m_topleft_client;
	driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
	gui::IGUIElement::draw();
}

bool GUISettingsMenu::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
			quitMenu();
			return true;
		}
		if(event.KeyInput.Key==KEY_RETURN && event.KeyInput.PressedDown)
		{
			quitMenu();
			return true;
		}
	}
	if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
	{
		switch(event.GUIEvent.Caller->getID())
		{
		case GUI_ID_CHANGE_VOLUME_BUTTON:
			quitMenu();
			m_gamecallback->changeVolume();
			return true;
		case GUI_ID_CHANGE_KEYS_BUTTON:
			quitMenu();
			m_gamecallback->changeKeys();
			return true;
		}
	}
	return Parent ? Parent->OnEvent(event) : false;
}


/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#ifndef HUD_HEADER
#define HUD_HEADER

#include "irrlichttypes_extrabloated.h"
#include <string>

#define HUD_DIR_LEFT_RIGHT 0
#define HUD_DIR_RIGHT_LEFT 1
#define HUD_DIR_TOP_BOTTOM 2
#define HUD_DIR_BOTTOM_TOP 3

#define HUD_CORNER_UPPER  0
#define HUD_CORNER_LOWER  1
#define HUD_CORNER_CENTER 2

#define HUD_FLAG_HOTBAR_VISIBLE    (1 << 0)
#define HUD_FLAG_HEALTHBAR_VISIBLE (1 << 1)
#define HUD_FLAG_CROSSHAIR_VISIBLE (1 << 2)
#define HUD_FLAG_WIELDITEM_VISIBLE (1 << 3)
#define HUD_FLAG_BREATHBAR_VISIBLE (1 << 4)

#define HUD_PARAM_HOTBAR_ITEMCOUNT 1
#define HUD_PARAM_HOTBAR_IMAGE 2
#define HUD_PARAM_HOTBAR_SELECTED_IMAGE 3

#define HUD_HOTBAR_ITEMCOUNT_DEFAULT 8
#define HUD_HOTBAR_ITEMCOUNT_MAX     23


#define HOTBAR_IMAGE_SIZE 48

enum HudElementType {
	HUD_ELEM_IMAGE     = 0,
	HUD_ELEM_TEXT      = 1,
	HUD_ELEM_STATBAR   = 2,
	HUD_ELEM_INVENTORY = 3,
	HUD_ELEM_WAYPOINT  = 4,
};

enum HudElementStat {
	HUD_STAT_POS = 0,
	HUD_STAT_NAME,
	HUD_STAT_SCALE,
	HUD_STAT_TEXT,
	HUD_STAT_NUMBER,
	HUD_STAT_ITEM,
	HUD_STAT_DIR,
	HUD_STAT_ALIGN,
	HUD_STAT_OFFSET,
	HUD_STAT_WORLD_POS,
	HUD_STAT_SIZE
};

struct HudElement {
	HudElementType type;
	v2f pos;
	std::string name;
	v2f scale;
	std::string text;
	u32 number;
	u32 item;
	u32 dir;
	v2f align;
	v2f offset;
	v3f world_pos;
	v2s32 size;
};

#ifndef SERVER

#include <vector>
#include <IGUIFont.h>
#include "irr_aabb3d.h"

class IGameDef;
class ITextureSource;
class Inventory;
class InventoryList;
class LocalPlayer;
struct ItemStack;

class Hud {
public:
	video::IVideoDriver *driver;
	scene::ISceneManager* smgr;
	gui::IGUIEnvironment *guienv;
	gui::IGUIFont *font;
	u32 text_height;
	IGameDef *gamedef;
	LocalPlayer *player;
	Inventory *inventory;
	ITextureSource *tsrc;

	video::SColor crosshair_argb;
	video::SColor selectionbox_argb;
	bool use_crosshair_image;
	std::string hotbar_image;
	bool use_hotbar_image;
	std::string hotbar_selected_image;
	bool use_hotbar_selected_image;
	v3s16 camera_offset;
	
	Hud(video::IVideoDriver *driver,scene::ISceneManager* smgr,
		gui::IGUIEnvironment* guienv, gui::IGUIFont *font,
		u32 text_height, IGameDef *gamedef,
		LocalPlayer *player, Inventory *inventory);
	
	void drawHotbar(u16 playeritem);
	void resizeHotbar();
	void drawCrosshair();
	void drawSelectionBoxes(std::vector<aabb3f> &hilightboxes);
	void drawLuaElements(v3s16 camera_offset);
private:
	void drawStatbar(v2s32 pos, u16 corner, u16 drawdir, std::string texture,
			s32 count, v2s32 offset, v2s32 size=v2s32());
	
	void drawItems(v2s32 upperleftpos, s32 itemcount, s32 offset,
		InventoryList *mainlist, u16 selectitem, u16 direction);

	void drawItem(const ItemStack &item, const core::rect<s32>& rect, bool selected);
	
	v2u32 m_screensize;
	v2s32 m_displaycenter;
	s32 m_hotbar_imagesize;
	s32 m_padding;
	video::SColor hbar_colors[4];
};

void drawItemStack(video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		IGameDef *gamedef);


#endif

#endif

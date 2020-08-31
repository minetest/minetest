/*
Minetest
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
Copyright (C) 2017 red-001 <red-001@outlook.ie>

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

#include <vector>
#include <IGUIFont.h>
#include "irr_aabb3d.h"
#include "../hud.h"

class Client;
class ITextureSource;
class Inventory;
class InventoryList;
class LocalPlayer;
struct ItemStack;

class Hud
{
public:
	video::IVideoDriver *driver;
	scene::ISceneManager *smgr;
	gui::IGUIEnvironment *guienv;
	Client *client;
	LocalPlayer *player;
	Inventory *inventory;
	ITextureSource *tsrc;

	video::SColor crosshair_argb;
	video::SColor selectionbox_argb;

	bool use_crosshair_image = false;
	bool use_object_crosshair_image = false;
	std::string hotbar_image = "";
	bool use_hotbar_image = false;
	std::string hotbar_selected_image = "";
	bool use_hotbar_selected_image = false;

	bool pointing_at_object = false;

	Hud(gui::IGUIEnvironment *guienv, Client *client, LocalPlayer *player,
			Inventory *inventory);
	~Hud();

	void drawHotbar(u16 playeritem);
	void resizeHotbar();
	void drawCrosshair();
	void drawSelectionMesh();
	void updateSelectionMesh(const v3s16 &camera_offset);

	std::vector<aabb3f> *getSelectionBoxes() { return &m_selection_boxes; }

	void setSelectionPos(const v3f &pos, const v3s16 &camera_offset);

	v3f getSelectionPos() const { return m_selection_pos; }

	void setSelectionMeshColor(const video::SColor &color)
	{
		m_selection_mesh_color = color;
	}

	void setSelectedFaceNormal(const v3f &face_normal)
	{
		m_selected_face_normal = face_normal;
	}

	void drawLuaElements(const v3s16 &camera_offset);

private:
	bool calculateScreenPos(const v3s16 &camera_offset, HudElement *e, v2s32 *pos);
	void drawStatbar(v2s32 pos, u16 corner, u16 drawdir,
			const std::string &texture, const std::string& bgtexture,
			s32 count, s32 maxcount, v2s32 offset, v2s32 size = v2s32());

	void drawItems(v2s32 upperleftpos, v2s32 screen_offset, s32 itemcount,
			s32 inv_offset, InventoryList *mainlist, u16 selectitem,
			u16 direction);

	void drawItem(const ItemStack &item, const core::rect<s32> &rect, bool selected);

	void drawCompassTranslate(HudElement *e, video::ITexture *texture,
			const core::rect<s32> &rect, int way);

	void drawCompassRotate(HudElement *e, video::ITexture *texture,
			const core::rect<s32> &rect, int way);

	float m_hud_scaling; // cached minetest setting
	float m_scale_factor;
	v3s16 m_camera_offset;
	v2u32 m_screensize;
	v2s32 m_displaycenter;
	s32 m_hotbar_imagesize; // Takes hud_scaling into account, updated by resizeHotbar()
	s32 m_padding; // Takes hud_scaling into account, updated by resizeHotbar()
	video::SColor hbar_colors[4];

	std::vector<aabb3f> m_selection_boxes;
	std::vector<aabb3f> m_halo_boxes;
	v3f m_selection_pos;
	v3f m_selection_pos_with_offset;

	scene::IMesh *m_selection_mesh = nullptr;
	video::SColor m_selection_mesh_color;
	v3f m_selected_face_normal;

	video::SMaterial m_selection_material;

	scene::SMeshBuffer m_rotation_mesh_buffer;

	enum
	{
		HIGHLIGHT_BOX,
		HIGHLIGHT_HALO,
		HIGHLIGHT_NONE
	} m_mode;
};

enum ItemRotationKind
{
	IT_ROT_SELECTED,
	IT_ROT_HOVERED,
	IT_ROT_DRAGGED,
	IT_ROT_OTHER,
	IT_ROT_NONE, // Must be last, also serves as number
};

void drawItemStack(video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		Client *client,
		ItemRotationKind rotation_kind);

void drawItemStack(
		video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		Client *client,
		ItemRotationKind rotation_kind,
		const v3s16 &angle,
		const v3s16 &rotation_speed);


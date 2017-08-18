/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 numzero, Lobachesky Vitaly <numzer0@yandex.ru>

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
#include "irrlichttypes_extrabloated.h"

class Camera;
class Client;
class Hud;
class Minimap;

class RenderingCore
{
protected:
	Camera *camera;
	Client *client;
	Minimap *mapper;
	Hud *hud;

	v2u32 screensize;
	video::SColor skycolor;
	bool show_hud;
	bool show_minimap;
	bool draw_wield_tool;
	bool draw_crosshair;

	IrrlichtDevice *device;
	video::IVideoDriver *driver;
	scene::ISceneManager *smgr;
	gui::IGUIEnvironment *guienv;

	void update_screen_size();
	virtual void init_textures() {}
	virtual void clear_textures() {}

	virtual void pre_draw() {}
	virtual void draw_all() = 0;
	virtual void post_draw() {}

	void draw_3d();
	void draw_hud();
	void draw_last_fx();

public:
	RenderingCore(IrrlichtDevice *_device);
	RenderingCore(const RenderingCore &) = delete;
	RenderingCore(RenderingCore &&) = delete;
	virtual ~RenderingCore();

	RenderingCore &operator= (const RenderingCore &) = delete;
	RenderingCore &operator= (RenderingCore &&) = delete;

	void initialize(Client *_client, Hud *_hud);
	void draw(video::SColor _skycolor, bool _show_hud, bool _show_minimap,
			bool _draw_wield_tool, bool _draw_crosshair);
};

class RenderingCorePlain: public RenderingCore
{
public:
	using RenderingCore::RenderingCore;
	void draw_all() override;
};

class RenderingCoreStereo: public RenderingCore
{
protected:
	scene::ICameraSceneNode *cam;
	core::matrix4 base_transform;
	float parallax_strength;

	void pre_draw();
	virtual void use_eye(bool right);
	virtual void reset_eye();
	void render_two();

public:
	RenderingCoreStereo(IrrlichtDevice *_device);
};

class RenderingCoreAnaglyph: public RenderingCoreStereo
{
protected:
	void setup_material(int color_mask);
	void use_eye(bool right) override;
	void reset_eye() override;

public:
	using RenderingCoreStereo::RenderingCoreStereo;
	void draw_all() override;
};

class RenderingCoreSideBySide: public RenderingCoreStereo
{
protected:
	video::ITexture *left = nullptr;
	video::ITexture *right = nullptr;
	video::ITexture *hud = nullptr;

	void init_textures() override;
	void clear_textures() override;
	void use_eye(bool right) override;
	void reset_eye() override;

public:
	using RenderingCoreStereo::RenderingCoreStereo;
	void draw_all() override;
};

class RenderingCorePageflip: public RenderingCoreStereo
{
protected:
	video::ITexture *hud = nullptr;

	void init_textures() override;
	void clear_textures() override;
	void use_eye(bool right) override;
	void reset_eye() override;

public:
	using RenderingCoreStereo::RenderingCoreStereo;
	void draw_all() override;
};

class RenderingCoreInterlaced: public RenderingCoreStereo
{
protected:
	video::ITexture *left = nullptr;
	video::ITexture *right = nullptr;
	video::ITexture *mask = nullptr;
	video::SMaterial mat;

	void init_material();
	void init_textures() override;
	void clear_textures() override;
	void init_mask();
	void use_eye(bool right) override;
	void reset_eye() override;
	void merge();

public:
	RenderingCoreInterlaced(IrrlichtDevice *_device);
	void draw_all() override;
};

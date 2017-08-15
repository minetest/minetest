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
class LocalPlayer;
class Hud;
class Minimap;

class RenderingCore
{
protected:
	Camera *camera;
	Client *client;
	LocalPlayer *player;
	Minimap *mapper;
	Hud *hud;

	v2u32 screensize;
	video::SColor skycolor;
	bool show_hud;
	bool show_minimap;
	bool draw_wield_tool;
	bool draw_crosshair;

	irr::IrrlichtDevice *device;
	irr::video::IVideoDriver *driver;
	irr::scene::ISceneManager *smgr;
	irr::gui::IGUIEnvironment *guienv;

public:
	RenderingCore(irr::IrrlichtDevice *_device);
	RenderingCore(const RenderingCore &) = delete;
	RenderingCore(RenderingCore &&) = delete;
	~RenderingCore() = default;

	RenderingCore &operator= (const RenderingCore &) = delete;
	RenderingCore &operator= (RenderingCore &&) = delete;

	void setup(Camera *_camera, Client *_client, LocalPlayer *_player,
		Hud *_hud, Minimap *_mapper, gui::IGUIEnvironment *_guienv,
		const v2u32 &_screensize, const video::SColor &_skycolor,
		bool _show_hud, bool _show_minimap);
	virtual void draw() = 0;

	void draw_3d();
	void draw_hud();
	void draw_last_fx();
};

class RenderingCorePlain: public RenderingCore
{
public:
	RenderingCorePlain(irr::IrrlichtDevice *_device);
	void draw() override;
};

class RenderingCoreStereo: public RenderingCore
{
protected:
	float parallax_strength;

public:
	enum class Eye
	{
		Left = -1,
		Right = 1,
	};
	RenderingCoreStereo(irr::IrrlichtDevice *_device);
	virtual void use_eye(Eye eye);
	virtual void use_default();
};

class RenderingCoreAnaglyph: public RenderingCoreStereo
{
public:
	RenderingCoreAnaglyph(irr::IrrlichtDevice *_device);
	void draw() override;
	void use_eye(Eye eye) override;
	void use_default() override;
};

class RenderingCoreDouble: public RenderingCoreStereo
{
public:
	enum class Mode
	{
		SideBySide,
		TopBottom,
		Pageflip
	};

	RenderingCoreDouble(irr::IrrlichtDevice *_device, Mode _mode);
	void draw() override;
};

class RenderingCoreInterlaced: public RenderingCoreStereo
{
public:
	RenderingCoreInterlaced(irr::IrrlichtDevice *_device);
	void draw() override;
};

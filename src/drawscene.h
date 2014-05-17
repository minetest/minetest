/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef DRAWSCENE_H_
#define DRAWSCENE_H_

#include "camera.h"
#include "hud.h"
#include "irrlichttypes_extrabloated.h"


void draw_load_screen(const std::wstring &text, IrrlichtDevice* device,
		gui::IGUIEnvironment* guienv, gui::IGUIFont* font, float dtime=0,
		int percent=0, bool clouds=true);

void draw_scene(video::IVideoDriver* driver, scene::ISceneManager* smgr,
		Camera& camera, Client& client, LocalPlayer* player, Hud& hud,
		gui::IGUIEnvironment* guienv, std::vector<aabb3f> hilightboxes,
		const v2u32& screensize, video::SColor skycolor, bool show_hud);

#endif /* DRAWSCENE_H_ */

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

#ifndef ADV3DMODE_H_
#define ADV3DMODE_H_

#include "camera.h"
#include "hud.h"
#include "irrlichttypes_extrabloated.h"

void draw_scene(Camera& camera, bool show_hud, Hud hud,
		std::vector<aabb3f> hilightboxes, video::IVideoDriver* driver,
		scene::ISceneManager* smgr,const v2u32& screensize,
		video::SColor skycolor, CameraMode cur_cam_mode, LocalPlayer* player,
		Client& client, gui::IGUIEnvironment* guienv);

#endif /* ADV_3D_MODE_H_ */

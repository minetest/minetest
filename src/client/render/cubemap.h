/*
Minetest
Copyright (C) 2019 srifqi, Muhammad Rifqi Priyo Susanto
		<muhammadrifqipriyosusanto@gmail.com>

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
#include "core.h"

class RenderingCoreCubeMap : public RenderingCore
{
protected:
	scene::ICameraSceneNode *cam;
	video::ITexture *faces[6];
	core::dimension2du image_size;

	void initTextures() override;
	void clearTextures() override;

	virtual void useFace(int face);
	virtual void drawFace(int face);

	void beforeDraw() override;
	void drawAll() override;

	void renderAllImages();

public:
	RenderingCoreCubeMap(IrrlichtDevice *_device, Client *_client, Hud *_hud);
};

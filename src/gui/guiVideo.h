/*
Minetest
Copyright (C) 2023 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

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

#include "client/videoplayer.h"

class GUIVideo : public gui::IGUIElement
{
public:
	GUIVideo(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const core::recti &rect, const std::string &filename);

	virtual void draw();

private:
	VideoPlayer          *m_video;
	video::IVideoDriver  *m_driver;
	u64 m_last_time      { 0 };
};

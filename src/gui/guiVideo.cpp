/*
Minetest
Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

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

#include "guiVideo.h"
#include "log.h"
#include "porting.h"
#include <string>

GUIVideo::GUIVideo(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
		s32 id, const core::rect<s32> &rectangle, const std::string &name) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
		m_name(name)
{
	VideoPlayer vp;
	vp.init(m_name.c_str(), m_device);
	m_frame = vp.getFrameTexture();
}

void GUIVideo::draw()
{
	const u32 now = m_device->getTimer()->getTime();
	if (!finished)
		vp.decodeFrame();

	if (((now - m_start_time) / 1000) > vp.getDuration())
		finished = true;

	m_frame = vp.getFrameTexture();

	if (!m_frame)
		return;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	u32 swidth = driver->getScreenSize().Width;
	u32 sheight = driver->getScreenSize().Height;
	u32 video_height = m_frame->getSize().Height;
	u32 video_width = m_frame->getSize().Width;

	if (!finished)
		driver->draw2DImage(m_frame, core::vector2di((swidth - video_width) / 2,
			(sheight - video_height) / 2));
}

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

#include "guiVideo.h"
#include "porting.h"

GUIVideo::GUIVideo(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const core::recti &rect, const std::string &filename)
	: IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rect)
{
	m_driver = Environment->getVideoDriver();
	m_video = new VideoPlayer();
	m_video->load(filename.c_str());
}

void GUIVideo::draw()
{
	if (!m_video)
		return;

	auto texture = m_video->getTexture();
	const auto &size = texture->getSize();
	auto rect_src = core::recti(0, 0, size.Width, size.Height);

	m_driver->draw2DImage(texture, getAbsoluteClippingRect(), rect_src);

	u64 new_time = porting::getTimeMs();
	u64 dtime_ms = 0;
	if (m_last_time != 0)
		dtime_ms = porting::getDeltaMs(m_last_time, new_time);
	m_last_time = new_time;

	m_video->update(dtime_ms);
}

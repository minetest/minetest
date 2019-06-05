/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "guiBackgroundImage.h"
#include "client/guiscalingfilter.h"
#include "log.h"

GUIBackgroundImage::GUIBackgroundImage(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent, s32 id, core::rect<s32> &rectangle,
		const std::string &name, const core::rect<s32> &middle,
		ISimpleTextureSource *tsrc) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
		m_name(name), m_middle(middle), m_tsrc(tsrc)
{
}

void GUIBackgroundImage::draw()
{
	if (!IsVisible)
		return;

	video::ITexture *texture = m_tsrc->getTexture(m_name);

	if (!texture) {
		errorstream << "GUIBackgroundImage::draw() Unable to load texture:"
			    << std::endl;
		errorstream << "\t" << m_name << std::endl;
		return;
	}

	video::IVideoDriver *driver = Environment->getVideoDriver();
	core::rect<s32> middle = m_middle;

	if (middle.getArea() == 0) {
		const video::SColor color(255, 255, 255, 255);
		const video::SColor colors[] = {color, color, color, color};
		draw2DImageFilterScaled(driver, texture, AbsoluteRect,
				core::rect<s32>(core::position2d<s32>(0, 0),
						core::dimension2di(
								texture->getOriginalSize())),
				&AbsoluteClippingRect, colors, true);
	} else {
		// `-x` is interpreted as `w - x`
		if (middle.LowerRightCorner.X < 0) {
			middle.LowerRightCorner.X += texture->getOriginalSize().Width;
		}
		if (middle.LowerRightCorner.Y < 0) {
			middle.LowerRightCorner.Y += texture->getOriginalSize().Height;
		}
		draw2DImage9Slice(driver, texture, AbsoluteRect, middle,
				&AbsoluteClippingRect);
	}

	IGUIElement::draw();
}

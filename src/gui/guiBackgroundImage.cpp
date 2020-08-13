/*
Part of Minetest
Copyright (C) 2013 RealBadAngel, Maciej Kasatkin <mk@realbadangel.pl>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "guiBackgroundImage.h"
#include "client/guiscalingfilter.h"
#include "log.h"

GUIBackgroundImage::GUIBackgroundImage(gui::IGUIEnvironment *env,
	gui::IGUIElement *parent, s32 id, const core::rect<s32> &rectangle,
	const std::string &name, const core::rect<s32> &middle,
	ISimpleTextureSource *tsrc, bool autoclip) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_name(name), m_middle(middle), m_tsrc(tsrc), m_autoclip(autoclip)
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

	core::rect<s32> rect = AbsoluteRect;
	if (m_autoclip)
		rect.LowerRightCorner += Parent->getAbsoluteClippingRect().getSize();

	video::IVideoDriver *driver = Environment->getVideoDriver();

	core::rect<s32> srcrect(core::position2d<s32>(0, 0),
			core::dimension2di(texture->getOriginalSize()));

	if (m_middle.getArea() == 0) {
		const video::SColor color(255, 255, 255, 255);
		const video::SColor colors[] = {color, color, color, color};
		draw2DImageFilterScaled(driver, texture, rect, srcrect, nullptr, colors, true);
	} else {
		draw2DImage9Slice(driver, texture, rect, srcrect, m_middle);
	}

	IGUIElement::draw();
}

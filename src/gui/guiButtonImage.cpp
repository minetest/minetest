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

#include "guiButtonImage.h"

#include "client/guiscalingfilter.h"
#include "debug.h"
#include "IGUIEnvironment.h"
#include "IGUIImage.h"
#include "IVideoDriver.h"
#include "StyleSpec.h"

using namespace irr;
using namespace gui;

GUIButtonImage::GUIButtonImage(gui::IGUIEnvironment *environment,
		gui::IGUIElement *parent, s32 id, core::rect<s32> rectangle, bool noclip)
	: GUIButton (environment, parent, id, rectangle, noclip)
{
	m_image = Environment->addImage(
			core::rect<s32>(0,0,rectangle.getWidth(),rectangle.getHeight()), this);
	m_image->setScaleImage(isScalingImage());
	sendToBack(m_image);
}

bool GUIButtonImage::OnEvent(const SEvent& event)
{
	bool result = GUIButton::OnEvent(event);

	EGUI_BUTTON_IMAGE_STATE imageState = getImageState(isPressed(), m_foreground_images);
	video::ITexture *texture = m_foreground_images[(u32)imageState].Texture;
	if (texture != nullptr)
	{
		m_image->setImage(texture);
	}

	m_image->setVisible(texture != nullptr);

	return result;
}

void GUIButtonImage::setForegroundImage(EGUI_BUTTON_IMAGE_STATE state,
		video::ITexture *image, const core::rect<s32> &sourceRect)
{
	if (state >= EGBIS_COUNT)
		return;

	if (image)
		image->grab();

	u32 stateIdx = (u32)state;
	if (m_foreground_images[stateIdx].Texture)
		m_foreground_images[stateIdx].Texture->drop();

	m_foreground_images[stateIdx].Texture = image;
	m_foreground_images[stateIdx].SourceRect = sourceRect;

	EGUI_BUTTON_IMAGE_STATE imageState = getImageState(isPressed(), m_foreground_images);
	if (imageState == stateIdx)
		m_image->setImage(image);
}

void GUIButtonImage::setForegroundImage(video::ITexture *image)
{
	setForegroundImage(gui::EGBIS_IMAGE_UP, image);
}

void GUIButtonImage::setForegroundImage(video::ITexture *image, const core::rect<s32> &pos)
{
	setForegroundImage(gui::EGBIS_IMAGE_UP, image, pos);
}

void GUIButtonImage::setPressedForegroundImage(video::ITexture *image)
{
	setForegroundImage(gui::EGBIS_IMAGE_DOWN, image);
}

void GUIButtonImage::setPressedForegroundImage(video::ITexture *image, const core::rect<s32> &pos)
{
	setForegroundImage(gui::EGBIS_IMAGE_DOWN, image, pos);
}

void GUIButtonImage::setHoveredForegroundImage(video::ITexture *image)
{
	setForegroundImage(gui::EGBIS_IMAGE_UP_MOUSEOVER, image);
	setForegroundImage(gui::EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER, image);
}

void GUIButtonImage::setHoveredForegroundImage(video::ITexture *image, const core::rect<s32> &pos)
{
	setForegroundImage(gui::EGBIS_IMAGE_UP_MOUSEOVER, image, pos);
	setForegroundImage(gui::EGBIS_IMAGE_UP_FOCUSED_MOUSEOVER, image, pos);
}

void GUIButtonImage::setFromStyle(const StyleSpec &style, ISimpleTextureSource *tsrc)
{
	GUIButton::setFromStyle(style, tsrc);

	video::IVideoDriver *driver = Environment->getVideoDriver();

	const core::position2di buttonCenter(AbsoluteRect.getCenter());
	core::position2d<s32> geom(buttonCenter);
	if (style.isNotDefault(StyleSpec::FGIMG)) {
		video::ITexture *texture = style.getTexture(StyleSpec::FGIMG, tsrc);

		setForegroundImage(guiScalingImageButton(driver, texture, geom.X, geom.Y));
		setScaleImage(true);
	}
	if (style.isNotDefault(StyleSpec::FGIMG_HOVERED)) {
		video::ITexture *hovered_texture = style.getTexture(StyleSpec::FGIMG_HOVERED, tsrc);

		setHoveredForegroundImage(guiScalingImageButton(driver, hovered_texture, geom.X, geom.Y));
		setScaleImage(true);
	}
	if (style.isNotDefault(StyleSpec::FGIMG_PRESSED)) {
		video::ITexture *pressed_texture = style.getTexture(StyleSpec::FGIMG_PRESSED, tsrc);

		setPressedForegroundImage(guiScalingImageButton(driver, pressed_texture, geom.X, geom.Y));
		setScaleImage(true);
	}
}

void GUIButtonImage::setScaleImage(bool scaleImage)
{
	GUIButton::setScaleImage(scaleImage);
	m_image->setScaleImage(scaleImage);
}

GUIButtonImage *GUIButtonImage::addButton(IGUIEnvironment *environment,
		const core::rect<s32> &rectangle, IGUIElement *parent, s32 id,
		const wchar_t *text, const wchar_t *tooltiptext)
{
	GUIButtonImage *button = new GUIButtonImage(environment,
			parent ? parent : environment->getRootGUIElement(), id, rectangle);

	if (text)
		button->setText(text);

	if (tooltiptext)
		button->setToolTipText(tooltiptext);

	button->drop();
	return button;
}

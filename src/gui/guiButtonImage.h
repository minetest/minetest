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

#include "guiButton.h"
#include "IGUIButton.h"

using namespace irr;

class GUIButtonImage : public GUIButton
{
public:
	//! constructor
	GUIButtonImage(gui::IGUIEnvironment *environment, gui::IGUIElement *parent,
			s32 id, core::rect<s32> rectangle, bool noclip = false);

	virtual bool OnEvent(const SEvent& event) override;

	void setForegroundImage(gui::EGUI_BUTTON_IMAGE_STATE state,
			video::ITexture *image = nullptr,
			const core::rect<s32> &sourceRect = core::rect<s32>(0, 0, 0, 0));

	void setForegroundImage(video::ITexture *image = nullptr);
	void setForegroundImage(video::ITexture *image, const core::rect<s32> &pos);

	void setPressedForegroundImage(video::ITexture *image = nullptr);
	void setPressedForegroundImage(video::ITexture *image, const core::rect<s32> &pos);

	void setHoveredForegroundImage(video::ITexture *image = nullptr);
	void setHoveredForegroundImage(video::ITexture *image, const core::rect<s32> &pos);

	virtual void setFromStyle(const StyleSpec &style, ISimpleTextureSource *tsrc) override;

	virtual void setScaleImage(bool scaleImage=true) override;

	//! Do not drop returned handle
	static GUIButtonImage *addButton(gui::IGUIEnvironment *environment,
			const core::rect<s32> &rectangle, IGUIElement *parent, s32 id,
			const wchar_t *text, const wchar_t *tooltiptext = L"");

private:
	ButtonImage m_foreground_images[gui::EGBIS_COUNT];
	gui::IGUIImage *m_image;
};

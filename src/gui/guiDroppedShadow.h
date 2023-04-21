/*
Alexios Avrassoglou <alexiosa@umich.edu>
Ashley David <asda@umich.edu>
*/


#ifndef GUI_BUTTON_ITEM_SHADOW_H
#define GUI_BUTTON_ITEM_SHADOW_H

#include <irrlicht.h>
#include "IGUISpriteBank.h"
#include "ITexture.h"
#include "SColor.h"
#include "Client.h"

namespace gui
{
	class ShadowBlockItem : public GUIButton
	{
	public:
		ShadowBlockItem(gui::IGUIEnvironment *environment,
			gui::IGUIElement *parent, s32 id, core::rect<s32> rectangle,
			ISimpleTextureSource *tsrc, const std::string &item,
			Client *client, bool noclip);

		static GUIShadowItemImage* addButton(IGUIEnvironment *environment,
			const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc,
			IGUIElement *parent, s32 id, const wchar_t *text,
			const std::string &item, Client *client, bool noclip = false);

		GUIItemShadowImage* getImage() const { return m_image; }

	private:
		GUIItemShadowImage *m_image;
		Client *m_client;
	};
}

#endif // GUI_BUTTON_ITEM_SHADOW_H

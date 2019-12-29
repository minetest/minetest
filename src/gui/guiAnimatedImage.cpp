#include "guiAnimatedImage.h"

#include "client/guiscalingfilter.h"
#include "client/tile.h" // ITextureSource
#include "client/texture_pool.h"
#include "log.h"

GUIAnimatedImage::GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
		s32 id, const core::rect<s32> &rectangle, const std::string &name,
		ISimpleTextureSource *tsrc, TexturePool *pool) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
		m_name(name), m_texture_idx(0), m_texture_pool(pool), m_tsrc(tsrc)
{
}

void GUIAnimatedImage::draw()
{
	video::ITexture *texture = m_texture_pool->getTexture(m_name, m_tsrc, &m_texture_idx);

	if (texture) {
		video::IVideoDriver *driver = Environment->getVideoDriver();

		const video::SColor color(255, 255, 255, 255);
		const video::SColor colors[] = {color, color, color, color};

		draw2DImageFilterScaled( driver, texture, AbsoluteRect,
				core::rect<s32>(core::position2d<s32>(0, 0), texture->getOriginalSize()),
				NoClip ? nullptr : &AbsoluteClippingRect, colors, true);
	}
}

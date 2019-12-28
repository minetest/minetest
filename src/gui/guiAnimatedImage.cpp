#include "guiAnimatedImage.h"

#include "client/guiscalingfilter.h"
#include "client/tile.h" // ITextureSource
#include "client/texture_pool.h"
#include "log.h"

GUIAnimatedImage::GUIAnimatedImage(gui::IGUIEnvironment *env,
	gui::IGUIElement *parent, s32 id, const core::rect<s32> &rectangle,
	const std::string &name, ISimpleTextureSource *tsrc, TexturePool* pool) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
	m_name(name),
	m_texture_idx(0),
	m_tsrc(tsrc),
	m_texture_pool(pool)
{
}

void GUIAnimatedImage::draw()
{
	video::ITexture *texture = m_texture_pool->getTexture(m_name, m_tsrc, m_texture_idx);

	if (texture) {
		video::IVideoDriver* driver = Environment->getVideoDriver();

		const core::dimension2d<u32> &img_origsize = texture->getOriginalSize();
		// Image size on screen
		core::rect<s32> imgrect = AbsoluteRect;

		// Image rectangle on screen
		const video::SColor color(255,255,255,255);
		const video::SColor colors[] = {color,color,color,color};

		draw2DImageFilterScaled(
				driver, texture, imgrect,
				core::rect<s32>(core::position2d<s32>(0,0), img_origsize),
				NULL, colors, true);
	} else {
		errorstream << "GUIAnimatedImage::draw() unable to load texture:" << std::endl;
		errorstream << "\t" << m_name << std::endl;
	}
}

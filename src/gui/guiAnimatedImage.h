#pragma once

#include "irrlichttypes_extrabloated.h"
#include "util/string.h"

class ISimpleTextureSource;
class TexturePool;

class GUIAnimatedImage : public gui::IGUIElement {
public:
	GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
			const core::rect<s32> &rectangle, const std::string &name,
			ISimpleTextureSource *tsrc, TexturePool* pool);

	virtual void draw() override;

private:
	std::string m_name;
	s32 m_texture_idx;
	TexturePool *m_texture_pool;
	ISimpleTextureSource *m_tsrc;
};

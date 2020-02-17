#pragma once

#include "irrlichttypes_extrabloated.h"
#include <string>

class ISimpleTextureSource;

class GUIAnimatedImage : public gui::IGUIElement {
public:
	GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
		const core::rect<s32> &rectangle, const std::string &name,
		ISimpleTextureSource *tsrc);

	virtual void draw() override;

private:
	ISimpleTextureSource *m_tsrc;

	video::ITexture *m_texture = nullptr;
	u64 m_global_time = 0;
	s32 m_frame_idx = 0;
	s32 m_frame_count = 1;
	u64 m_frame_duration = 1;
	u64 m_frame_time = 0;
};

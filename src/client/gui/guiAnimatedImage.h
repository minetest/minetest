#pragma once

#include "irrlichttypes_extrabloated.h"
#include <string>

class ISimpleTextureSource;

class GUIAnimatedImage : public gui::IGUIElement {
public:
	GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
		s32 id, const core::rect<s32> &rectangle, const std::string &texture_name,
		s32 frame_count, s32 frame_duration, ISimpleTextureSource *tsrc);

	virtual void draw() override;

	void setFrameIndex(s32 frame);
	s32 getFrameIndex() const { return m_frame_idx; };

private:
	ISimpleTextureSource *m_tsrc;

	video::ITexture *m_texture = nullptr;
	u64 m_global_time = 0;
	s32 m_frame_idx = 0;
	s32 m_frame_count = 1;
	u64 m_frame_duration = 1;
	u64 m_frame_time = 0;
};

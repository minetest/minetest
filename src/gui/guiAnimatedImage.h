#pragma once

#include "irrlichttypes_extrabloated.h"
#include "util/string.h"

class ISimpleTextureSource;

class GUIAnimatedImage : public gui::IGUIElement {
public:
	GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent, s32 id,
			const core::rect<s32> &rectangle, const std::string &name,
			ISimpleTextureSource *tsrc);

	virtual void draw() override;

private:
	std::string m_name;
	ISimpleTextureSource *m_tsrc;

	video::ITexture *m_texture;
	u64 m_global_time;
	s32 m_frame_idx;
	s32 m_frame_count;
	u64 m_frame_duration;
	u64 m_frame_time;
};

#include "guiAnimatedImage.h"

#include "client/guiscalingfilter.h"
#include "client/tile.h" // ITextureSource
#include "log.h"
#include "porting.h"
#include "util/string.h"
#include <string>
#include <vector>

GUIAnimatedImage::GUIAnimatedImage(gui::IGUIEnvironment *env, gui::IGUIElement *parent,
	s32 id, const core::rect<s32> &rectangle, const std::string &texture_name,
	s32 frame_count, s32 frame_duration, ISimpleTextureSource *tsrc) :
	gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle), m_tsrc(tsrc)
{
	m_texture = m_tsrc->getTexture(texture_name);

	m_frame_count    = std::max(frame_count,    1);
	m_frame_duration = std::max(frame_duration, 0);

	if (m_texture != nullptr) {
		core::dimension2d<u32> size = m_texture->getOriginalSize();
		if (size.Height < (u64)m_frame_count)
			m_frame_count = size.Height;
	} else {
		// No need to step an animation if we have nothing to draw
		m_frame_count = 1;
	}
}

void GUIAnimatedImage::draw()
{
	// Render the current frame
	if (m_texture != nullptr) {
		video::IVideoDriver *driver = Environment->getVideoDriver();

		const video::SColor color(255, 255, 255, 255);
		const video::SColor colors[] = {color, color, color, color};

		core::dimension2d<u32> size = m_texture->getOriginalSize();
		size.Height /= m_frame_count;

		draw2DImageFilterScaled(driver, m_texture, AbsoluteRect,
			core::rect<s32>(core::position2d<s32>(0, size.Height * m_frame_idx), size),
			NoClip ? nullptr : &AbsoluteClippingRect, colors, true);
	}

	// Step the animation
	if (m_frame_count > 1 && m_frame_duration > 0) {
		// Determine the delta time to step
		u64 new_global_time = porting::getTimeMs();
		if (m_global_time > 0)
			m_frame_time += new_global_time - m_global_time;

		m_global_time = new_global_time;

		// Advance by the number of elapsed frames, looping if necessary
		m_frame_idx += u32(m_frame_time / m_frame_duration);
		m_frame_idx %= m_frame_count;

		// If 1 or more frames have elapsed, reset the frame time counter with
		// the remainder
		m_frame_time %= m_frame_duration;
	}
}


void GUIAnimatedImage::setFrameIndex(s32 frame)
{
	s32 idx = std::max(frame, 0);
	if (idx > 0 && idx < m_frame_count)
		m_frame_idx = idx;
}

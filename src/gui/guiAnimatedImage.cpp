#include "guiAnimatedImage.h"

#include "client/guiscalingfilter.h"
#include "log.h"
#include "porting.h"
#include "util/string.h"
#include <string>
#include <vector>
#include "filesys.h"
#include "porting.h"
#include "client/texturesource.h"

GUIAnimatedImage::GUIAnimatedImage(gui::IGUIEnvironment *env, ISimpleTextureSource *tsrc, gui::IGUIElement *parent,
	s32 id, const core::rect<s32> &rectangle, bool allow_remote) :
		gui::IGUIElement(gui::EGUIET_ELEMENT, env, parent, id, rectangle),
		m_textureSource(tsrc),
		m_allow_remote_images(allow_remote),
		source(env->getVideoDriver(), env->getFileSystem())
{
}

void GUIAnimatedImage::setTexture(const std::string &image_name)
{
	if (str_starts_with(image_name, "https://") ||
		str_starts_with(image_name, "http://")) {
		if (m_allow_remote_images) {
#if USE_CURL
			source.load(image_name);
			pollRemoteImage();
#else
			errorstream << "Unable to load remote image as Minetest was no compiled with cURL" << std::endl;
			setTexture(nullptr);
#endif
		} else {
			errorstream << "http images are not permitted in-game" << std::endl;
			setTexture(nullptr);
		}
	} else {
		setTexture(m_textureSource->getTexture(image_name));
	}
}

void GUIAnimatedImage::draw()
{
	if (m_texture == nullptr)
		return;

	pollRemoteImage();

	video::IVideoDriver *driver = Environment->getVideoDriver();

	core::dimension2d<u32> size = m_texture->getOriginalSize();

	if ((u32)m_frame_count > size.Height)
		m_frame_count = size.Height;
	if (m_frame_idx >= m_frame_count)
		m_frame_idx = m_frame_count - 1;

	size.Height /= m_frame_count;

	core::rect<s32> rect(core::position2d<s32>(0, size.Height * m_frame_idx), size);
	core::rect<s32> *cliprect = NoClip ? nullptr : &AbsoluteClippingRect;

	if (m_middle.getArea() == 0) {
		const video::SColor color(255, 255, 255, 255);
		const video::SColor colors[] = {color, color, color, color};
		draw2DImageFilterScaled(driver, m_texture, AbsoluteRect, rect, cliprect,
			colors, true);
	} else {
		draw2DImage9Slice(driver, m_texture, AbsoluteRect, rect, m_middle, cliprect);
	}

	// Step the animation
	if (m_frame_count > 1 && m_frame_duration > 0) {
		// Determine the delta time to step
		u64 new_global_time = porting::getTimeMs();
		if (m_global_time > 0)
			m_frame_time += new_global_time - m_global_time;

		m_global_time = new_global_time;

		// Advance by the number of elapsed frames, looping if necessary
		m_frame_idx += (u32)(m_frame_time / m_frame_duration);
		m_frame_idx %= m_frame_count;

		// If 1 or more frames have elapsed, reset the frame time counter with
		// the remainder
		m_frame_time %= m_frame_duration;
	}
}


void GUIAnimatedImage::pollRemoteImage()
{
#if USE_CURL
	if (source.getState() == RemoteTexture::Empty)
		return;

	source.update();

	std::string textures_pack = fs::RemoveRelativePathComponents(
			porting::path_share + DIR_DELIM + "textures" + DIR_DELIM + "base" + DIR_DELIM + "pack") + DIR_DELIM;

	switch (source.getState()) {
		case RemoteTexture::Empty:
			break;
		case RemoteTexture::Loading: {
			auto texture = m_textureSource->getTexture(textures_pack + "loading_screenshot.png");
			if (texture != m_texture) {
				setTexture(texture);
			}
			break;
		}
		case RemoteTexture::Success: {
			auto texture = source.get();
			if (texture && texture != m_texture) {
				setTexture(texture.get());
			}
			break;
		}
		case RemoteTexture::Failure: {
			auto texture = m_textureSource->getTexture(textures_pack + "error_screenshot.png");
			if (texture != m_texture) {
				setTexture(texture);
			}
			break;
		}
	}
#endif
}

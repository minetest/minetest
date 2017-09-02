#include "texture.h"
#include "porting.h"
#include <sstream>

/*
	Texture
*/

void Texture::set(const std::string &name_, const std::string &base_name,
		s32 frame_count_, u64 frame_duration_)
{
	name = name_;
	frame_count = frame_count_;
	frame_index = 0;
	frame_duration = frame_duration_;
	frame_time = 0;
	frame_names.resize( frame_count );

	for (s32 i = 0; i < frame_count; ++i) {
		std::stringstream frame_name;
		frame_name << base_name << "_" << (i + 1) << ".png";
		frame_names[i] = frame_name.str();
	}
}


void Texture::set(const std::string &name_)
{
	name = name_;
	frame_count = 0;
	frame_index = 0;
	frame_duration = 0.;
	frame_time = 0;
	frame_names.resize(0);
}


video::ITexture *Texture::getTexture(ITextureSource *texture_source)
{
	if (frame_count > 0)
		return texture_source->getTexture(frame_names[frame_index]);
	else
		return texture_source->getTexture(name);
}


void Texture::step(u64 step_duration)
{
	frame_time += step_duration;
	frame_index += u32(frame_time / frame_duration);
	frame_index %= frame_count;
	frame_time %= frame_duration;
}

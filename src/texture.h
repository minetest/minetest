#pragma once

#include "irrlichttypes_extrabloated.h"
#include "client/tile.h"
#include <ITexture.h>
#include <string>
#include <vector>

/*
	Texture
*/

struct Texture
{
	Texture() : name(), frame_count(0), frame_index(0), frame_duration(0),
		frame_time(0), frame_names()
	{}

	Texture(const Texture &texture) : name(texture.name),
		frame_count(texture.frame_count),
		frame_index(texture.frame_index),
		frame_duration(texture.frame_duration),
		frame_time(texture.frame_time),
		frame_names(texture.frame_names)
	{}

	~Texture()
	{}

	void set(const std::string &name_, const std::string &base_name,
		s32 frame_count_, u64 frame_duration_);

	void set(const std::string &name_);

	video::ITexture *getTexture(ITextureSource *texture_source);

	void step(u64 step_duration);

	std::string name;
	s32 frame_count = 0;
	s32 frame_index = 0;
	u64 frame_duration = 0;
	u64 frame_time = 0;
	std::vector<std::string> frame_names;
};

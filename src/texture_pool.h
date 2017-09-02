#pragma once

#include "irrlichttypes_extrabloated.h"
#include "texture.h"
#include <string>
#include <vector>

/*
	Animation
*/

struct TexturePool
{
	TexturePool() :	textures(), animations(), global_time(0)
	{}

	TexturePool(const TexturePool &texture_pool) :
		textures(texture_pool.textures),
		animations(texture_pool.animations),
		global_time(texture_pool.global_time)
	{}

	~TexturePool()
	{}

	s32 addTexture(const std::string &name,
		const std::string &base_name, s32 frame_count,
		u64 frame_duration);

	s32 addTexture(const std::string &name);

	s32 getTextureIndex(const std::string &name);

	video::ITexture *getTexture(const std::string &name,
			ITextureSource *texture_source, s32 &texture_index);

	void step();

	/*
		Properties
	*/

	std::vector<Texture> textures;
	std::vector<s32> animations;
	u64 global_time = 0;
};

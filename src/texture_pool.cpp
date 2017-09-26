#include "texture_pool.h"
#include "porting.h"
#include <string>

/*
	TexturePool
*/

s32 TexturePool::addTexture(const std::string &name,
		const std::string &base_name, s32 frame_count,
		u64 frame_duration)
{
	s32 texture_index = textures.size() + 1;
	textures.resize(texture_index);
	textures[texture_index - 1].set(name, base_name, frame_count, frame_duration);
	animations.push_back(texture_index - 1);

	return texture_index;
}


s32 TexturePool::addTexture(const std::string &name)
{
	s32 texture_index = textures.size() + 1;
	textures.resize(texture_index);
	textures[texture_index - 1].set(name);

	return texture_index;
}


s32 TexturePool::getTextureIndex(const std::string &name)
{
	for (int i = 0, c = textures.size(); i < c; ++i) {
		if (textures[i].name == name)
			return i + 1;
	}

	std::string::size_type colon_position = name.find( ":", 0 );
	std::string::size_type comma_position = name.find( ",", 0 );

	if (comma_position > colon_position
		&& comma_position < name.size()) {
		std::string base_name = name.substr(0, colon_position);
		s32 frame_count = std::stoi(name.substr(colon_position + 1, comma_position - colon_position - 1));
		s32 frame_duration = std::stoi(name.substr(comma_position + 1));
		if (base_name.size() > 0 && frame_count > 0 && frame_duration > 0)
			return addTexture(name, base_name, frame_count, frame_duration);
		else
			return addTexture(base_name);
	}
	else
		return addTexture(name);
}


video::ITexture *TexturePool::getTexture(const std::string &name,
	ITextureSource *texture_source, s32 &texture_index)
{
	if (name.size() == 0)
		return texture_source->getTexture(name);

	if (texture_index == 0)
		texture_index = getTextureIndex(name);

	return textures[texture_index - 1].getTexture(texture_source);
}


void TexturePool::step()
{
	u64 new_global_time = porting::getTimeMs();
	u64 step_duration;

	if ( global_time == 0 )
		step_duration = 0;
	else
		step_duration = new_global_time - global_time;

	global_time = new_global_time;

	for (int i = 0, c = animations.size(); i < c; ++i) {
		textures[animations[i]].step(step_duration);
	}
}

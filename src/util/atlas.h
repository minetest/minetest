/*
Minetest
Copyright (C) 2015 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef __ATLAS_H__
#define __ATLAS_H__
#include "irrlichttypes_bloated.h"
#include <ITexture.h>
#include <string>
#include <vector>

class IGameDef;

namespace irr {
	namespace video {
		class IVideoDriver;
	}
}

namespace atlas
{
	static const size_t ATLAS_UNDEFINED = 0;

	struct AtlasSegmentReference
	{
		size_t atlas_id; // 0 = undefined atlas
		size_t segment_id;

		AtlasSegmentReference():
			atlas_id(ATLAS_UNDEFINED),
			segment_id(0)
		{}
	};

	const uint16_t ATLAS_LOD_TOP_FACE = 0x100;
	const uint16_t ATLAS_LOD_SEMIBRIGHT1_FACE = 0x200;
	const uint16_t ATLAS_LOD_SEMIBRIGHT2_FACE = 0x400;
	const uint16_t ATLAS_LOD_BAKE_SHADOWS = 0x800;
	const uint16_t ATLAS_LOD_DARKEN_LIKE_LIQUID = 0x1600;

	struct AtlasSegmentDefinition
	{
		std::string image_name; // If "", segment won't be added
		v2s32 total_segments;
		v2s32 select_segment;
		v2s32 target_size;
		// Mask 0x00ff: LOD level, mask 0xff00: flags
		uint16_t lod_simulation;
		// TODO: Rotation

		AtlasSegmentDefinition():
			lod_simulation(0)
		{}

		bool operator==(const AtlasSegmentDefinition &other) const;
	};

	struct AtlasSegmentCache
	{
		video::ITexture **texture_pp;
		v2f coord0;
		v2f coord1;

		AtlasSegmentCache():
			texture_pp(NULL)
		{}
	};

	struct AtlasDefinition
	{
		size_t id;
		v2s32 segment_resolution;
		v2s32 total_segments;
		std::vector<AtlasSegmentDefinition> segments;

		AtlasDefinition():
			id(ATLAS_UNDEFINED)
		{}
	};

	struct AtlasCache
	{
		size_t id;
		video::IImage *image;
		std::string texture_name;
		mutable video::ITexture *texture;
		v2s32 segment_resolution;
		v2s32 total_segments;
		mutable std::vector<AtlasSegmentCache> segments;

		AtlasCache():
			id(ATLAS_UNDEFINED),
			image(NULL),
			texture(NULL)
		{}
		AtlasCache(const AtlasCache &other):
			id(other.id),
			image(other.image),
			texture_name(other.texture_name),
			texture(other.texture),
			segment_resolution(other.segment_resolution),
			total_segments(other.total_segments),
			segments(other.segments)
		{
			if (image)   image->grab();
			if (texture) texture->grab();
		}
		~AtlasCache() {
			if (image)   image->drop();
			if (texture) texture->drop();
		}
	};

	struct AtlasRegistry
	{
		virtual ~AtlasRegistry(){}

		// Allows the atlas to optimize itself for upcoming segmnets
		virtual void prepare_for_segments(
				size_t num_segments, v2s32 segment_size) = 0;

		// These two may only be called from Urho3D main thread
		virtual const AtlasSegmentReference add_segment(
				const AtlasSegmentDefinition &segment_def) = 0;
		virtual const AtlasSegmentReference find_or_add_segment(
				const AtlasSegmentDefinition &segment_def) = 0;

		virtual const AtlasDefinition* get_atlas_definition(
				size_t atlas_id) = 0;
		virtual const AtlasSegmentDefinition* get_segment_definition(
				const AtlasSegmentReference &ref) = 0;

		virtual void refresh_textures() = 0;

		virtual const AtlasCache* get_atlas_cache(size_t atlas_id) = 0;

		virtual const AtlasSegmentCache* get_texture(
				const AtlasSegmentReference &ref) = 0;
	};

	AtlasRegistry* createAtlasRegistry(const std::string &name,
			video::IVideoDriver* driver, IGameDef *gamedef);
}

#endif

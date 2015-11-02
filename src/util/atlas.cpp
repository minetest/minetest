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

#include "atlas.h"
#include "string.h"
#include "log.h"
#include "../gamedef.h"
#include "../client/tile.h" // ITextureSource
#include "../exceptions.h"
#include "irrlichttypes_extrabloated.h"

namespace atlas {

bool AtlasSegmentDefinition::operator==(const AtlasSegmentDefinition &other) const
{
	return (
			image_name == other.image_name &&
			total_segments == other.total_segments &&
			select_segment == other.select_segment &&
			lod_simulation == other.lod_simulation
	);
}

struct CAtlasRegistry: public AtlasRegistry
{
	std::string m_name; // Required for unique texture name generation in Irrlicht
	video::IVideoDriver *m_driver;
	IGameDef *m_gamedef;
	std::vector<AtlasDefinition> m_defs;
	std::vector<AtlasCache> m_cache;
	v2u32 m_max_texture_resolution;

	CAtlasRegistry(const std::string &name, video::IVideoDriver *driver,
			IGameDef *gamedef):
		m_name(name),
		m_driver(driver),
		m_gamedef(gamedef)
	{
		m_defs.resize(1); // id=0 is ATLAS_UNDEFINED
		m_max_texture_resolution = driver->getMaxTextureSize();
		infostream<<"AtlasRegistry: Maximum texture resolution: "
				<<m_max_texture_resolution.X<<"x"
				<<m_max_texture_resolution.Y<<std::endl;
	}

	void prepare_for_segments(size_t num_segments, v2s32 segment_size)
	{
		// Lessen the maximum texture size if we don't need such a large one
		// The size needed is 2x2 the segment size due to padding that is used
		// for avoiding texture bleeding
		size_t size_needed = 2 * npot2(ceilf(sqrt(
				segment_size.X * segment_size.Y * num_segments)));
		if (m_max_texture_resolution.X > size_needed)
			m_max_texture_resolution.X = size_needed;
		if (m_max_texture_resolution.Y > size_needed)
			m_max_texture_resolution.Y = size_needed;
		infostream<<"AtlasRegistry: Optimized texture resolution: "
				<<m_max_texture_resolution.X<<"x"
				<<m_max_texture_resolution.Y<<std::endl;
	}

	video::IImage* textureToImage(video::ITexture* texture)
	{
		if (!texture) return NULL;
		video::IImage* image = m_driver->createImageFromData(
				texture->getColorFormat(),
				texture->getSize(),
				texture->lock(),
				false, // ownForeignMemory
				false // deleteMemory
		);
		texture->unlock();
		return image;
	}

	const AtlasSegmentReference add_segment(
			const AtlasSegmentDefinition &segment_def)
	{
		ITextureSource *tsrc = m_gamedef->getTextureSource();

		// TODO: Maybe creating an image out of a texture isn't a very good idea
		//       as images are the actual in-memory source format
		//video::ITexture *seg_tex = tsrc->getTexture(segment_def.image_name);
		//video::IImage *seg_img = textureToImage(seg_tex);

		video::IImage *seg_img = tsrc->generateImage(segment_def.image_name);

		// Get resolution of texture
		v2s32 seg_img_size(seg_img->getDimension().Width, seg_img->getDimension().Height);
		if (segment_def.target_size != v2s32(0, 0)) {
			// Override target size as requested
			seg_img_size = segment_def.target_size;
		} else if (segment_def.lod_simulation >= 2) {
			// Force the same size for everything in order to optimize number of
			// texture atlases
			seg_img_size.X = 64;
			seg_img_size.Y = 64;
		}
		// Try to find a texture atlas for this texture size
		AtlasDefinition *atlas_def = NULL;
		for(size_t i=0; i<m_defs.size(); i++){
			AtlasDefinition &def0 = m_defs[i];
			if(def0.id == ATLAS_UNDEFINED)
				continue;
			if(def0.segment_resolution == seg_img_size){
				size_t max = def0.total_segments.X * def0.total_segments.Y;
				if(def0.segments.size() >= max){
					/*log_d(MODULE, "add_segment(): Found atlas for segment size "
							"(%i, %i) %p, but it is full",
							seg_img_size.X, seg_img_size.Y, &def0);*/
					continue; // Full
				}
				atlas_def = &def0;
				break;
			}
		}
		// If not found, create a texture atlas for this texture size
		if(atlas_def){
			/*log_d(MODULE, "add_segment(): Found atlas for segment size "
					"(%i, %i): %p", seg_img_size.X, seg_img_size.Y, atlas_def);*/
		} else {
			// Create a new texture atlas
			m_defs.resize(m_defs.size()+1);
			atlas_def = &m_defs[m_defs.size()-1];
			/*log_d(MODULE, "add_segment(): Creating atlas for segment size "
					"(%i, %i): %p", seg_img_size.X, seg_img_size.Y, atlas_def);*/
			atlas_def->id = m_defs.size()-1;
			if(segment_def.total_segments.X == 0 ||
					segment_def.total_segments.Y == 0)
				throw BaseException("segment_def.total_segments is zero");
			// Calculate segment resolution
			v2s32 seg_res(
					seg_img_size.X / segment_def.total_segments.X,
					seg_img_size.Y / segment_def.total_segments.Y
			);
			atlas_def->segment_resolution = seg_res;
			// Calculate total segments based on segment resolution
			atlas_def->total_segments = v2s32(
					m_max_texture_resolution.X / seg_res.X / 2,
					m_max_texture_resolution.Y / seg_res.Y / 2
			);
			v2s32 atlas_resolution(
					atlas_def->total_segments.X * seg_res.X * 2,
					atlas_def->total_segments.Y * seg_res.Y * 2
			);
			// Create image for new atlas
			video::IImage *atlas_img = m_driver->createImage(video::ECF_A8R8G8B8,
					core::dimension2d<u32>(atlas_resolution.X, atlas_resolution.Y));
			// Add new atlas to cache
			const size_t &id = atlas_def->id;
			m_cache.resize(id+1);
			AtlasCache &atlas_cache = m_cache[id];
			atlas_cache.id = id;
			atlas_cache.image = atlas_img;
			atlas_cache.image->grab();
			atlas_cache.texture = NULL;
			atlas_cache.texture_name = std::string()+"atlas_"+m_name+"_texture_"+itos(id);
			atlas_cache.segment_resolution = atlas_def->segment_resolution;
			atlas_cache.total_segments = atlas_def->total_segments;
		}
		// Add this segment to the atlas definition
		size_t seg_id = atlas_def->segments.size();
		atlas_def->segments.resize(seg_id + 1);
		atlas_def->segments[seg_id] = segment_def;
		// Update this segment in cache
		AtlasCache &atlas_cache = m_cache[atlas_def->id];
		atlas_cache.segments.resize(seg_id + 1);
		AtlasSegmentCache &seg_cache = atlas_cache.segments[seg_id];
		update_segment_cache(seg_id, seg_img, seg_cache, segment_def, atlas_cache);
		/*// DEBUG
		static int si = 0;
		std::string path = std::string()+"/tmp/seg_img_"+itos(si++)+".png";
		m_driver->writeImageToFile(seg_img, path.c_str(), 0);*/
		seg_img->drop();
		// Return reference to new segment
		AtlasSegmentReference ref;
		ref.atlas_id = atlas_def->id;
		ref.segment_id = seg_id;
		return ref;
	}

	const AtlasSegmentReference find_or_add_segment(
			const AtlasSegmentDefinition &segment_def)
	{
		// Find an atlas that contains this segment; return reference if found
		for(size_t i=0; i<m_defs.size(); i++){
			AtlasDefinition &atlas_def = m_defs[i];
			for(size_t seg_id = 0; seg_id<atlas_def.segments.size(); seg_id++){
				AtlasSegmentDefinition &segment_def0 = atlas_def.segments[seg_id];
				if(segment_def0 == segment_def){
					AtlasSegmentReference ref;
					ref.atlas_id = atlas_def.id;
					ref.segment_id = seg_id;
					return ref;
				}
			}
		}
		// Segment was not found; add a new one
		return add_segment(segment_def);
	}

	const AtlasDefinition* get_atlas_definition(size_t atlas_id)
	{
		if(atlas_id == ATLAS_UNDEFINED)
			return NULL;
		if(atlas_id >= m_defs.size())
			return NULL;
		return &m_defs[atlas_id];
	}

	const AtlasSegmentDefinition* get_segment_definition(
			const AtlasSegmentReference &ref)
	{
		const AtlasDefinition *atlas = get_atlas_definition(ref.atlas_id);
		if(!atlas)
			return NULL;
		if(ref.segment_id >= atlas->segments.size())
			return NULL;
		return &atlas->segments[ref.segment_id];
	}

	void update_segment_cache(size_t seg_id, video::IImage *seg_img,
			AtlasSegmentCache &cache, const AtlasSegmentDefinition &def,
			const AtlasCache &acache)
	{
		// Check if atlas has too many segments
		size_t max_segments = acache.total_segments.X * acache.total_segments.Y;
		if(acache.segments.size() > max_segments){
			throw BaseException("Atlas has too many segments (segments.size()="+
					itos(acache.segments.size())+", total_segments=("+
					itos(acache.total_segments.X)+", "+
					itos(acache.total_segments.Y)+"))");
		}
		// Calculate segment's position in atlas texture
		v2s32 total_segs = acache.total_segments;
		size_t seg_iy = seg_id / total_segs.X;
		size_t seg_ix = seg_id - seg_iy * total_segs.X;
		/*log_d(MODULE, "update_segment_cache(): seg_id=%i, seg_iy=%i, seg_ix=%i",
				seg_id, seg_iy, seg_ix);*/
		v2s32 seg_size = acache.segment_resolution;
		v2s32 dst_p00(
				seg_ix * seg_size.X * 2,
				seg_iy * seg_size.Y * 2
		);
		v2s32 dst_p0 = dst_p00 + seg_size / 2;
		v2s32 dst_p1 = dst_p0 + seg_size;
		// Set coordinates in cache
		cache.coord0 = v2f(
				(float)dst_p0.X / (float)(total_segs.X * seg_size.X * 2),
				(float)dst_p0.Y / (float)(total_segs.Y * seg_size.Y * 2)
		);
		cache.coord1 = v2f(
				(float)dst_p1.X / (float)(total_segs.X * seg_size.X * 2),
				(float)dst_p1.Y / (float)(total_segs.Y * seg_size.Y * 2)
		);
		// Draw segment into atlas image
		v2s32 src_size(seg_img->getDimension().Width, seg_img->getDimension().Height);
		v2s32 src_off(
				src_size.X / def.total_segments.X * def.select_segment.X,
				src_size.Y / def.total_segments.Y * def.select_segment.Y
		);
		// Draw main texture
		if(def.lod_simulation == 0){
			for(int y = 0; y<seg_size.Y * 2; y++){
				for(int x = 0; x<seg_size.X * 2; x++){
					/*float fx = (float)src_size.X / seg_size.X;
					float fy = (float)src_size.Y / seg_size.Y;
					v2s32 src_p = src_off + v2s32(
							(int)(x * fx + src_size.X / 2) % src_size.X,
							(int)(y * fy + src_size.Y / 2) % src_size.Y
					);*/
					// TODO: Not sure if this is right
					v2s32 src_p = src_off + v2s32(
							(x + seg_size.X / 2) % seg_size.X,
							(y + seg_size.Y / 2) % seg_size.Y
					);
					v2s32 dst_p = dst_p00 + v2s32(x, y);
					video::SColor c = seg_img->getPixel(src_p.X, src_p.Y);
					if (src_p.X % 8 == 0 && src_p.Y % 8 == 0) {
						infostream<<"seg_img->getPixel("<<src_p.X<<","<<src_p.Y
								<<") -> r="<<c.getRed()<<",g="<<c.getGreen()
								<<",b="<<c.getBlue()<<",a="<<c.getAlpha()
								<<std::endl;
					}
					//c.set(255, 128, 128, 128); // DEBUG
					acache.image->setPixel(dst_p.X, dst_p.Y, c);
				}
			}
		} else {
			int lod = def.lod_simulation & 0x00ff;
			uint16_t flags = def.lod_simulation & 0xff00;

			// Cache for an on-demand search of an opaque pixel
			bool any_opaque_color_searched = false;
			video::SColor any_opaque_color;
			any_opaque_color.setAlpha(0);

			for(int y = 0; y<seg_size.Y * 2; y++){
				for(int x = 0; x<seg_size.X * 2; x++){
					float fx = (float)src_size.X * lod / seg_size.X;
					float fy = (float)src_size.Y * lod / seg_size.Y;
					// TODO: Not sure if this is right
					v2s32 src_p = src_off + v2s32(
							(int)(x * fx + src_size.X * lod / 2) % src_size.X,
							(int)(y * fy + src_size.Y * lod / 2) % src_size.Y
					);
					/*v2s32 src_p = src_off + v2s32(
							((x + seg_size.X / 2) * lod) % seg_size.X,
							((y + seg_size.Y / 2) * lod) % seg_size.Y
					);*/
					v2s32 dst_p = dst_p00 + v2s32(x, y);

					video::SColor c = seg_img->getPixel(src_p.X, src_p.Y);

					// Try to get a pixel that isn't transparent
					if (c.getAlpha() < 255) {
						video::SColor c2;
						c2.setAlpha(0);
						for (s32 x = src_p.X-1; x <= src_p.X + 1 &&
								c2.getAlpha() < 255; x++)
						for (s32 y = src_p.Y-1; y <= src_p.Y + 1 &&
								c2.getAlpha() < 255; y++) {
							if (x < 0 || x >= src_size.X ||
									y < 0 || y > src_size.Y)
								continue;
							c2 = seg_img->getPixel(x, y);
						}
						if (c2.getAlpha() == 255) {
							c = c2;
						} else if(any_opaque_color_searched) {
							if (any_opaque_color.getAlpha() != 0) {
								c = any_opaque_color;
							} else {
								// We don't have any
							}
						} else {
							any_opaque_color_searched = true;
							// Well, crap. Search for the whole source texture
							// for a non-transparent pixel then. There's
							// probably at least one in it, and if there isn't,
							// at least we tried.
							for (s32 x = src_off.X; x < src_off.X + src_size.X &&
									c2.getAlpha() < 255; x++)
							for (s32 y = src_off.Y; y < src_off.Y + src_size.Y &&
									c2.getAlpha() < 255; y++) {
								c2 = seg_img->getPixel(x, y);
							}
							if (c2.getAlpha() == 255) {
								c = c2;
								any_opaque_color = c2;
							} else {
								// Tough luck
							}
						}
					}

					if(flags & ATLAS_LOD_TOP_FACE){
						// Preserve original colors
						if(flags & ATLAS_LOD_BAKE_SHADOWS){
							// This value has been calibrated
							c.set(
								c.getAlpha(),
								c.getRed() * 0.97f,
								c.getGreen() * 0.97f,
								c.getBlue() * 0.99f
							);
						} else {
							// This value has been calibrated
							c.set(
								c.getAlpha(),
								c.getRed() * 0.97f,
								c.getGreen() * 0.97f,
								c.getBlue() * 1.0f
							);
						}
					} else {
						// Simulate sides
						// Leave horizontal edges look like they are bright
						// topsides
						// TODO: This should be variable according to the
						// camera's height relative to the thing the atlas
						// segment is representing
						int edge_size = src_size.Y / 16;
						bool is_edge = (
								src_p.Y <= edge_size ||
								src_p.Y >= src_size.Y - edge_size
						);
						if(flags & ATLAS_LOD_BAKE_SHADOWS){
							if(is_edge){
								if(flags & ATLAS_LOD_SEMIBRIGHT1_FACE){
									c.set(
										c.getAlpha(),
										c.getRed() * 0.75f,
										c.getGreen() * 0.75f,
										c.getBlue() * 0.8f
									);
								} else if(flags & ATLAS_LOD_SEMIBRIGHT2_FACE){
									c.set(
										c.getAlpha(),
										c.getRed() * 0.75f,
										c.getGreen() * 0.75f,
										c.getBlue() * 0.8f
									);
								} else {
									// This value has been calibrated
									c.set(
										c.getAlpha(),
										c.getRed() * 0.97f * 0.9f,
										c.getGreen() * 0.97f * 0.9f,
										c.getBlue() * 1.0f * 0.9f
									);
								}
							} else {
								if(flags & ATLAS_LOD_SEMIBRIGHT1_FACE){
									c.set(
										c.getAlpha(),
										c.getRed() * 0.70f * 0.75f,
										c.getGreen() * 0.70f * 0.75f,
										c.getBlue() * 0.65f * 0.8f
									);
								} else if(flags & ATLAS_LOD_SEMIBRIGHT2_FACE){
									c.set(
										c.getAlpha(),
										c.getRed() * 0.50f * 0.75f,
										c.getGreen() * 0.50f * 0.75f,
										c.getBlue() * 0.50f * 0.8f
									);
								} else {
									// This value has been calibrated
									c.set(
										c.getAlpha(),
										c.getRed() * 0.61f * 1.0f,
										c.getGreen() * 0.61f * 1.0f,
										c.getBlue() * 0.67f * 1.0f
									);
								}
							}
						} else {
							if(is_edge){
								// This value has been calibrated
								c.set(
									c.getAlpha(),
									c.getRed() * 0.97f,
									c.getGreen() * 0.97f,
									c.getBlue() * 1.0f
								);
							} else {
								if(flags & ATLAS_LOD_SEMIBRIGHT1_FACE){
									c.set(
										c.getAlpha(),
										c.getRed() * 0.7f,
										c.getGreen() * 0.7f,
										c.getBlue() * 0.7f
									);
								} else if(flags & ATLAS_LOD_SEMIBRIGHT2_FACE){
									c.set(
										c.getAlpha(),
										c.getRed() * 0.7f,
										c.getGreen() * 0.7f,
										c.getBlue() * 0.7f
									);
								} else {
									// This value has been calibrated
									c.set(
										c.getAlpha(),
										c.getRed() * 0.7f,
										c.getGreen() * 0.7f,
										c.getBlue() * 0.8f
									);
								}
							}
						}
					}

					if(flags & ATLAS_LOD_DARKEN_LIKE_LIQUID){
						// This value has been calibrated
						c.set(
							c.getAlpha(),
							c.getRed() * 0.5,
							c.getGreen() * 0.5,
							c.getBlue() * 0.5
						);
					}

					acache.image->setPixel(dst_p.X, dst_p.Y, c);
				}
			}
		}
		// Drop old texture so that a new one will be generated
		if (acache.texture != NULL) {
			infostream<<"Dropping acache.texture "<<acache.texture<<std::endl;
			acache.texture->drop();
			acache.texture = NULL;
			for(size_t i=0; i<acache.segments.size(); i++){
				AtlasSegmentCache &scache = acache.segments[i];
				scache.texture_pp = NULL;
			}
		}
	}

	void refresh_textures()
	{
		for(size_t i=0; i<m_cache.size(); i++){
			AtlasCache &acache = m_cache[i];
			if (acache.texture != NULL)
				continue;
			if (acache.image == NULL)
				continue;

			// Uncomment for debugging
			/*std::string path = std::string()+"/tmp/"+acache.texture_name+".png";
			m_driver->writeImageToFile(acache.image, path.c_str(), 0);*/

			acache.texture = m_driver->addTexture(acache.texture_name.c_str(),
					acache.image);
			acache.texture->grab();
			for(size_t i=0; i<acache.segments.size(); i++){
				AtlasSegmentCache &scache = acache.segments[i];
				scache.texture_pp = &acache.texture;
			}
			infostream<<"Created acache.texture "<<acache.texture<<std::endl;
		}
	}

	const AtlasCache* get_atlas_cache(size_t atlas_id)
	{
		if(atlas_id == ATLAS_UNDEFINED)
			return NULL;
		if(atlas_id >= m_cache.size()){
			// Cache is always up-to-date
			return NULL;
		}
		return &m_cache[atlas_id];
	}

	const AtlasSegmentCache* get_texture(const AtlasSegmentReference &ref)
	{
		const AtlasCache *cache = get_atlas_cache(ref.atlas_id);
		if(cache == NULL)
			return NULL;
		if(ref.segment_id >= cache->segments.size()){
			// Cache is always up-to-date
			return NULL;
		}
		const AtlasSegmentCache &seg_cache = cache->segments[ref.segment_id];
		return &seg_cache;
	}
};

AtlasRegistry* createAtlasRegistry(const std::string &name,
		video::IVideoDriver* driver, IGameDef *gamedef)
{
	return new CAtlasRegistry(name, driver, gamedef);
}

}

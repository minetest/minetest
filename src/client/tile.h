/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

#include "irrlichttypes.h"
#include "irr_v3d.h"
#include <ITexture.h>
#include <string>
#include <vector>
#include <SMaterial.h>
#include "util/numeric.h"
#include "config.h"

#if ENABLE_GLES
#include <IVideoDriver.h>
#endif

class IGameDef;
struct TileSpec;
struct TileDef;

typedef std::vector<video::SColor> Palette;

/*
	tile.{h,cpp}: Texture handling stuff.
*/

/*
	Find out the full path of an image by trying different filename
	extensions.

	If failed, return "".

	TODO: Should probably be moved out from here, because things needing
	      this function do not need anything else from this header
*/
std::string getImagePath(std::string path);

/*
	Gets the path to a texture by first checking if the texture exists
	in texture_path and if not, using the data path.

	Checks all supported extensions by replacing the original extension.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getTexturePath(const std::string &filename, bool *is_base_pack = nullptr);

void clearTextureNameCache();

/*
	TextureSource creates and caches textures.
*/

class ISimpleTextureSource
{
public:
	ISimpleTextureSource() = default;

	virtual ~ISimpleTextureSource() = default;

	virtual video::ITexture* getTexture(
			const std::string &name, u32 *id = nullptr) = 0;
};

class ITextureSource : public ISimpleTextureSource
{
public:
	ITextureSource() = default;

	virtual ~ITextureSource() = default;

	virtual u32 getTextureId(const std::string &name)=0;
	virtual std::string getTextureName(u32 id)=0;
	virtual video::ITexture* getTexture(u32 id)=0;
	virtual video::ITexture* getTexture(
			const std::string &name, u32 *id = nullptr)=0;
	virtual video::ITexture* getTextureForMesh(
			const std::string &name, u32 *id = nullptr) = 0;
	/*!
	 * Returns a palette from the given texture name.
	 * The pointer is valid until the texture source is
	 * destructed.
	 * Should be called from the main thread.
	 */
	virtual Palette* getPalette(const std::string &name) = 0;
	virtual bool isKnownSourceImage(const std::string &name)=0;
	virtual video::ITexture* getNormalTexture(const std::string &name)=0;
	virtual video::SColor getTextureAverageColor(const std::string &name)=0;
	virtual video::ITexture *getShaderFlagsTexture(bool normalmap_present)=0;
};

class IWritableTextureSource : public ITextureSource
{
public:
	IWritableTextureSource() = default;

	virtual ~IWritableTextureSource() = default;

	virtual u32 getTextureId(const std::string &name)=0;
	virtual std::string getTextureName(u32 id)=0;
	virtual video::ITexture* getTexture(u32 id)=0;
	virtual video::ITexture* getTexture(
			const std::string &name, u32 *id = nullptr)=0;
	virtual bool isKnownSourceImage(const std::string &name)=0;

	virtual void processQueue()=0;
	virtual void insertSourceImage(const std::string &name, video::IImage *img)=0;
	virtual void rebuildImagesAndTextures()=0;
	virtual video::ITexture* getNormalTexture(const std::string &name)=0;
	virtual video::SColor getTextureAverageColor(const std::string &name)=0;
	virtual video::ITexture *getShaderFlagsTexture(bool normalmap_present)=0;
};

IWritableTextureSource *createTextureSource();

#if ENABLE_GLES
bool hasNPotSupport();
video::IImage * Align2Npot2(video::IImage * image, irr::video::IVideoDriver* driver);
#endif

enum MaterialType{
	TILE_MATERIAL_BASIC,
	TILE_MATERIAL_ALPHA,
	TILE_MATERIAL_LIQUID_TRANSPARENT,
	TILE_MATERIAL_LIQUID_OPAQUE,
	TILE_MATERIAL_WAVING_LEAVES,
	TILE_MATERIAL_WAVING_PLANTS,
	TILE_MATERIAL_OPAQUE,
	TILE_MATERIAL_WAVING_LIQUID_BASIC,
	TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT,
	TILE_MATERIAL_WAVING_LIQUID_OPAQUE,
	TILE_MATERIAL_PLAIN,
	TILE_MATERIAL_PLAIN_ALPHA
};

// Material flags
// Should backface culling be enabled?
#define MATERIAL_FLAG_BACKFACE_CULLING 0x01
// Should a crack be drawn?
#define MATERIAL_FLAG_CRACK 0x02
// Should the crack be drawn on transparent pixels (unset) or not (set)?
// Ignored if MATERIAL_FLAG_CRACK is not set.
#define MATERIAL_FLAG_CRACK_OVERLAY 0x04
#define MATERIAL_FLAG_ANIMATION 0x08
//#define MATERIAL_FLAG_HIGHLIGHTED 0x10
#define MATERIAL_FLAG_TILEABLE_HORIZONTAL 0x20
#define MATERIAL_FLAG_TILEABLE_VERTICAL 0x40

/*
	This fully defines the looks of a tile.
	The SMaterial of a tile is constructed according to this.
*/
struct FrameSpec
{
	FrameSpec() = default;

	u32 texture_id = 0;
	video::ITexture *texture = nullptr;
	video::ITexture *normal_texture = nullptr;
	video::ITexture *flags_texture = nullptr;
};

#define MAX_TILE_LAYERS 2

//! Defines a layer of a tile.
struct TileLayer
{
	TileLayer() = default;

	/*!
	 * Two layers are equal if they can be merged.
	 */
	bool operator==(const TileLayer &other) const
	{
		return
			texture_id == other.texture_id &&
			material_type == other.material_type &&
			material_flags == other.material_flags &&
			color == other.color &&
			scale == other.scale;
	}

	/*!
	 * Two tiles are not equal if they must have different vertices.
	 */
	bool operator!=(const TileLayer &other) const
	{
		return !(*this == other);
	}

	// Sets everything else except the texture in the material
	void applyMaterialOptions(video::SMaterial &material) const
	{
		switch (material_type) {
		case TILE_MATERIAL_OPAQUE:
		case TILE_MATERIAL_LIQUID_OPAQUE:
		case TILE_MATERIAL_WAVING_LIQUID_OPAQUE:
			material.MaterialType = video::EMT_SOLID;
			break;
		case TILE_MATERIAL_BASIC:
		case TILE_MATERIAL_WAVING_LEAVES:
		case TILE_MATERIAL_WAVING_PLANTS:
		case TILE_MATERIAL_WAVING_LIQUID_BASIC:
			material.MaterialTypeParam = 0.5;
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			break;
		case TILE_MATERIAL_ALPHA:
		case TILE_MATERIAL_LIQUID_TRANSPARENT:
		case TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT:
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			break;
		default:
			break;
		}
		material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) != 0;
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
			material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		}
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
			material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		}
	}

	void applyMaterialOptionsWithShaders(video::SMaterial &material) const
	{
		material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) != 0;
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
			material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
			material.TextureLayer[1].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		}
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
			material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
			material.TextureLayer[1].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		}
	}

	bool isTileable() const
	{
		return (material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)
			&& (material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL);
	}

	// Ordered for size, please do not reorder

	video::ITexture *texture = nullptr;
	video::ITexture *normal_texture = nullptr;
	video::ITexture *flags_texture = nullptr;

	u32 shader_id = 0;

	u32 texture_id = 0;

	u16 animation_frame_length_ms = 0;
	u16 animation_frame_count = 1;

	u8 material_type = TILE_MATERIAL_BASIC;
	u8 material_flags =
		//0 // <- DEBUG, Use the one below
		MATERIAL_FLAG_BACKFACE_CULLING |
		MATERIAL_FLAG_TILEABLE_HORIZONTAL|
		MATERIAL_FLAG_TILEABLE_VERTICAL;

	//! If true, the tile has its own color.
	bool has_color = false;

	std::vector<FrameSpec> *frames = nullptr;

	/*!
	 * The color of the tile, or if the tile does not own
	 * a color then the color of the node owning this tile.
	 */
	video::SColor color;

	u8 scale;
};

/*!
 * Defines a face of a node. May have up to two layers.
 */
struct TileSpec
{
	TileSpec() = default;

	/*!
	 * Returns true if this tile can be merged with the other tile.
	 */
	bool isTileable(const TileSpec &other) const {
		for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
			if (layers[layer] != other.layers[layer])
				return false;
			if (!layers[layer].isTileable())
				return false;
		}
		return rotation == 0
			&& rotation == other.rotation
			&& emissive_light == other.emissive_light;
	}

	//! If true, the tile rotation is ignored.
	bool world_aligned = false;
	//! Tile rotation.
	u8 rotation = 0;
	//! This much light does the tile emit.
	u8 emissive_light = 0;
	//! The first is base texture, the second is overlay.
	TileLayer layers[MAX_TILE_LAYERS];
};

std::vector<std::string> getTextureDirs();

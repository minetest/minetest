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

#ifndef TILE_HEADER
#define TILE_HEADER

#include "irrlichttypes.h"
#include "irr_v3d.h"
#include <ITexture.h>
#include <IrrlichtDevice.h>
#include "threads.h"
#include <string>
#include <vector>
#include "util/numeric.h"

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
std::string getTexturePath(const std::string &filename);

void clearTextureNameCache();

/*
	ITextureSource::generateTextureFromMesh parameters
*/
namespace irr {namespace scene {class IMesh;}}
struct TextureFromMeshParams
{
	scene::IMesh *mesh;
	core::dimension2d<u32> dim;
	std::string rtt_texture_name;
	bool delete_texture_on_shutdown;
	v3f camera_position;
	v3f camera_lookat;
	core::CMatrix4<f32> camera_projection_matrix;
	video::SColorf ambient_light;
	v3f light_position;
	video::SColorf light_color;
	f32 light_radius;
};

/*
	TextureSource creates and caches textures.
*/

class ISimpleTextureSource
{
public:
	ISimpleTextureSource(){}
	virtual ~ISimpleTextureSource(){}
	virtual video::ITexture* getTexture(
			const std::string &name, u32 *id = NULL) = 0;
};

class ITextureSource : public ISimpleTextureSource
{
public:
	ITextureSource(){}
	virtual ~ITextureSource(){}
	virtual u32 getTextureId(const std::string &name)=0;
	virtual std::string getTextureName(u32 id)=0;
	virtual video::ITexture* getTexture(u32 id)=0;
	virtual video::ITexture* getTexture(
			const std::string &name, u32 *id = NULL)=0;
	virtual video::ITexture* getTextureForMesh(
			const std::string &name, u32 *id = NULL) = 0;
	/*!
	 * Returns a palette from the given texture name.
	 * The pointer is valid until the texture source is
	 * destructed.
	 * Should be called from the main thread.
	 */
	virtual Palette* getPalette(const std::string &name) = 0;
	virtual IrrlichtDevice* getDevice()=0;
	virtual bool isKnownSourceImage(const std::string &name)=0;
	virtual video::ITexture* generateTextureFromMesh(
			const TextureFromMeshParams &params)=0;
	virtual video::ITexture* getNormalTexture(const std::string &name)=0;
	virtual video::SColor getTextureAverageColor(const std::string &name)=0;
	virtual video::ITexture *getShaderFlagsTexture(bool normalmap_present)=0;
};

class IWritableTextureSource : public ITextureSource
{
public:
	IWritableTextureSource(){}
	virtual ~IWritableTextureSource(){}
	virtual u32 getTextureId(const std::string &name)=0;
	virtual std::string getTextureName(u32 id)=0;
	virtual video::ITexture* getTexture(u32 id)=0;
	virtual video::ITexture* getTexture(
			const std::string &name, u32 *id = NULL)=0;
	virtual IrrlichtDevice* getDevice()=0;
	virtual bool isKnownSourceImage(const std::string &name)=0;
	virtual video::ITexture* generateTextureFromMesh(
			const TextureFromMeshParams &params)=0;

	virtual void processQueue()=0;
	virtual void insertSourceImage(const std::string &name, video::IImage *img)=0;
	virtual void rebuildImagesAndTextures()=0;
	virtual video::ITexture* getNormalTexture(const std::string &name)=0;
	virtual video::SColor getTextureAverageColor(const std::string &name)=0;
	virtual video::ITexture *getShaderFlagsTexture(bool normalmap_present)=0;
};

IWritableTextureSource* createTextureSource(IrrlichtDevice *device);

#ifdef __ANDROID__
video::IImage * Align2Npot2(video::IImage * image, video::IVideoDriver* driver);
#endif

enum MaterialType{
	TILE_MATERIAL_BASIC,
	TILE_MATERIAL_ALPHA,
	TILE_MATERIAL_LIQUID_TRANSPARENT,
	TILE_MATERIAL_LIQUID_OPAQUE,
	TILE_MATERIAL_WAVING_LEAVES,
	TILE_MATERIAL_WAVING_PLANTS
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
#define MATERIAL_FLAG_HIGHLIGHTED 0x10
#define MATERIAL_FLAG_TILEABLE_HORIZONTAL 0x20
#define MATERIAL_FLAG_TILEABLE_VERTICAL 0x40

/*
	This fully defines the looks of a tile.
	The SMaterial of a tile is constructed according to this.
*/
struct FrameSpec
{
	FrameSpec():
		texture_id(0),
		texture(NULL),
		normal_texture(NULL),
		flags_texture(NULL)
	{
	}
	u32 texture_id;
	video::ITexture *texture;
	video::ITexture *normal_texture;
	video::ITexture *flags_texture;
};

struct TileSpec
{
	TileSpec():
		texture_id(0),
		texture(NULL),
		normal_texture(NULL),
		flags_texture(NULL),
		material_type(TILE_MATERIAL_BASIC),
		material_flags(
			//0 // <- DEBUG, Use the one below
			MATERIAL_FLAG_BACKFACE_CULLING
		),
		shader_id(0),
		animation_frame_count(1),
		animation_frame_length_ms(0),
		rotation(0),
		has_color(false),
		color(),
		emissive_light(0)
	{
	}

	/*!
	 * Two tiles are equal if they can be appended to
	 * the same mesh buffer.
	 */
	bool operator==(const TileSpec &other) const
	{
		return (
			texture_id == other.texture_id &&
			material_type == other.material_type &&
			material_flags == other.material_flags &&
			rotation == other.rotation
		);
	}

	/*!
	 * Two tiles are not equal if they must be in different mesh buffers.
	 */
	bool operator!=(const TileSpec &other) const
	{
		return !(*this == other);
	}
	
	// Sets everything else except the texture in the material
	void applyMaterialOptions(video::SMaterial &material) const
	{
		switch (material_type) {
		case TILE_MATERIAL_BASIC:
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			break;
		case TILE_MATERIAL_ALPHA:
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			break;
		case TILE_MATERIAL_LIQUID_TRANSPARENT:
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			break;
		case TILE_MATERIAL_LIQUID_OPAQUE:
			material.MaterialType = video::EMT_SOLID;
			break;
		case TILE_MATERIAL_WAVING_LEAVES:
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			break;
		case TILE_MATERIAL_WAVING_PLANTS:
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
			break;
		}
		material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING)
			? true : false;
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
			material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		}
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
			material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		}
	}

	void applyMaterialOptionsWithShaders(video::SMaterial &material) const
	{
		material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING)
			? true : false;
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_HORIZONTAL)) {
			material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
			material.TextureLayer[1].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		}
		if (!(material_flags & MATERIAL_FLAG_TILEABLE_VERTICAL)) {
			material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
			material.TextureLayer[1].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		}
	}
	
	u32 texture_id;
	video::ITexture *texture;
	video::ITexture *normal_texture;
	video::ITexture *flags_texture;
	
	// Material parameters
	u8 material_type;
	u8 material_flags;
	u32 shader_id;
	// Animation parameters
	u8 animation_frame_count;
	u16 animation_frame_length_ms;
	std::vector<FrameSpec> frames;

	u8 rotation;
	//! If true, the tile has its own color.
	bool has_color;
	/*!
	 * The color of the tile, or if the tile does not own
	 * a color then the color of the node owning this tile.
	 */
	video::SColor color;
	//! This much light does the tile emit.
	u8 emissive_light;
};
#endif

/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "irr_v2d.h"
#include "irr_v3d.h"
#include <ITexture.h>
#include <IrrlichtDevice.h>
#include "threads.h"
#include <string>

class IGameDef;

/*
	tile.{h,cpp}: Texture handling stuff.
*/

/*
	Gets the path to a texture by first checking if the texture exists
	in texture_path and if not, using the data path.

	Checks all supported extensions by replacing the original extension.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getTexturePath(const std::string &filename);

/*
	Specifies a texture in an atlas.

	This is used to specify single textures also.

	This has been designed to be small enough to be thrown around a lot.
*/
struct AtlasPointer
{
	u32 id; // Texture id
	video::ITexture *atlas; // Atlas in where the texture is
	v2f pos; // Position in atlas
	v2f size; // Size in atlas
	u16 tiled; // X-wise tiling count. If 0, width of atlas is width of image.

	AtlasPointer():
		id(0),
		atlas(NULL),
		pos(0,0),
		size(1,1),
		tiled(1)
	{}

	AtlasPointer(
			u16 id_,
			video::ITexture *atlas_=NULL,
			v2f pos_=v2f(0,0),
			v2f size_=v2f(1,1),
			u16 tiled_=1
		):
		id(id_),
		atlas(atlas_),
		pos(pos_),
		size(size_),
		tiled(tiled_)
	{
	}

	bool operator==(const AtlasPointer &other) const
	{
		return (
			id == other.id
		);
		/*return (
			id == other.id &&
			atlas == other.atlas &&
			pos == other.pos &&
			size == other.size &&
			tiled == other.tiled
		);*/
	}

	bool operator!=(const AtlasPointer &other) const
	{
		return !(*this == other);
	}

	float x0(){ return pos.X; }
	float x1(){ return pos.X + size.X; }
	float y0(){ return pos.Y; }
	float y1(){ return pos.Y + size.Y; }
};

/*
	TextureSource creates and caches textures.
*/

class ITextureSource
{
public:
	ITextureSource(){}
	virtual ~ITextureSource(){}
	virtual u32 getTextureId(const std::string &name){return 0;}
	virtual u32 getTextureIdDirect(const std::string &name){return 0;}
	virtual std::string getTextureName(u32 id){return "";}
	virtual AtlasPointer getTexture(u32 id){return AtlasPointer(0);}
	virtual AtlasPointer getTexture(const std::string &name)
		{return AtlasPointer(0);}
	virtual video::ITexture* getTextureRaw(const std::string &name)
		{return NULL;}
	virtual AtlasPointer getTextureRawAP(const AtlasPointer &ap)
		{return AtlasPointer(0);}
	virtual IrrlichtDevice* getDevice()
		{return NULL;}
	virtual void updateAP(AtlasPointer &ap){};
};

class IWritableTextureSource : public ITextureSource
{
public:
	IWritableTextureSource(){}
	virtual ~IWritableTextureSource(){}
	virtual u32 getTextureId(const std::string &name){return 0;}
	virtual u32 getTextureIdDirect(const std::string &name){return 0;}
	virtual std::string getTextureName(u32 id){return "";}
	virtual AtlasPointer getTexture(u32 id){return AtlasPointer(0);}
	virtual AtlasPointer getTexture(const std::string &name)
		{return AtlasPointer(0);}
	virtual video::ITexture* getTextureRaw(const std::string &name)
		{return NULL;}
	virtual IrrlichtDevice* getDevice()
		{return NULL;}
	virtual void updateAP(AtlasPointer &ap){};

	virtual void processQueue()=0;
	virtual void insertSourceImage(const std::string &name, video::IImage *img)=0;
	virtual void rebuildImagesAndTextures()=0;
	virtual void buildMainAtlas(class IGameDef *gamedef)=0;
};

IWritableTextureSource* createTextureSource(IrrlichtDevice *device);

enum MaterialType{
	MATERIAL_ALPHA_NONE,
	MATERIAL_ALPHA_VERTEX,
	MATERIAL_ALPHA_SIMPLE, // >127 = opaque
	MATERIAL_ALPHA_BLEND,
};

// Material flags
// Should backface culling be enabled?
#define MATERIAL_FLAG_BACKFACE_CULLING 0x01
// Should a crack be drawn?
#define MATERIAL_FLAG_CRACK 0x02
// Should the crack be drawn on transparent pixels (unset) or not (set)?
// Ignored if MATERIAL_FLAG_CRACK is not set.
#define MATERIAL_FLAG_CRACK_OVERLAY 0x04
// Animation made up by splitting the texture to vertical frames, as
// defined by extra parameters
#define MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES 0x08

/*
	This fully defines the looks of a tile.
	The SMaterial of a tile is constructed according to this.
*/
struct TileSpec
{
	TileSpec():
		texture(0),
		alpha(255),
		//material_type(MATERIAL_ALPHA_NONE),
		// Use this so that leaves don't need a separate material
		material_type(MATERIAL_ALPHA_SIMPLE),
		material_flags(
			//0 // <- DEBUG, Use the one below
			MATERIAL_FLAG_BACKFACE_CULLING
		),
		animation_frame_count(1),
		animation_frame_length_ms(0)
	{
	}

	bool operator==(const TileSpec &other) const
	{
		return (
			texture == other.texture &&
			alpha == other.alpha &&
			material_type == other.material_type &&
			material_flags == other.material_flags
		);
	}

	bool operator!=(const TileSpec &other) const
	{
		return !(*this == other);
	}
	
	// Sets everything else except the texture in the material
	void applyMaterialOptions(video::SMaterial &material) const
	{
		if(material_type == MATERIAL_ALPHA_NONE)
			material.MaterialType = video::EMT_SOLID;
		else if(material_type == MATERIAL_ALPHA_VERTEX)
			material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
		else if(material_type == MATERIAL_ALPHA_SIMPLE)
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		else if(material_type == MATERIAL_ALPHA_BLEND)
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

		material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) ? true : false;
	}
	
	// NOTE: Deprecated, i guess?
	void setTexturePos(u8 tx_, u8 ty_, u8 tw_, u8 th_)
	{
		texture.pos = v2f((float)tx_/256.0, (float)ty_/256.0);
		texture.size = v2f(((float)tw_ + 1.0)/256.0, ((float)th_ + 1.0)/256.0);
	}
	
	AtlasPointer texture;
	// Vertex alpha (when MATERIAL_ALPHA_VERTEX is used)
	u8 alpha;
	// Material parameters
	u8 material_type;
	u8 material_flags;
	// Animation parameters
	u8 animation_frame_count;
	u16 animation_frame_length_ms;
};

#endif

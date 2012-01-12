/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TILE_HEADER
#define TILE_HEADER

#include "common_irrlicht.h"
#include "threads.h"
#include "utility.h"
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

	bool operator==(const AtlasPointer &other)
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
#define MATERIAL_FLAG_BACKFACE_CULLING 0x01

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
		)
	{
	}

	bool operator==(TileSpec &other)
	{
		return (
			texture == other.texture &&
			alpha == other.alpha &&
			material_type == other.material_type &&
			material_flags == other.material_flags
		);
	}
	
	// Sets everything else except the texture in the material
	void applyMaterialOptions(video::SMaterial &material) const
	{
		if(alpha != 255 && material_type != MATERIAL_ALPHA_VERTEX)
			dstream<<"WARNING: TileSpec: alpha != 255 "
					"but not MATERIAL_ALPHA_VERTEX"
					<<std::endl;

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
	// Vertex alpha
	u8 alpha;
	// Material type
	u8 material_type;
	// Material flags
	u8 material_flags;
};

#endif

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
	An internal variant of the former with more data.
*/
struct SourceAtlasPointer
{
	std::string name;
	AtlasPointer a;
	video::IImage *atlas_img; // The source image of the atlas
	// Integer variants of position and size
	v2s32 intpos;
	v2u32 intsize;

	SourceAtlasPointer(
			const std::string &name_,
			AtlasPointer a_=AtlasPointer(0, NULL),
			video::IImage *atlas_img_=NULL,
			v2s32 intpos_=v2s32(0,0),
			v2u32 intsize_=v2u32(0,0)
		):
		name(name_),
		a(a_),
		atlas_img(atlas_img_),
		intpos(intpos_),
		intsize(intsize_)
	{
	}
};

/*
	Implementation (to be used as a no-op on the server)
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
};

/*
	Creates and caches textures.
*/
class TextureSource : public ITextureSource
{
public:
	TextureSource(IrrlichtDevice *device);
	~TextureSource();

	/*
		Processes queued texture requests from other threads.

		Shall be called from the main thread.
	*/
	void processQueue();
	
	/*
		Example case:
		Now, assume a texture with the id 1 exists, and has the name
		"stone.png^mineral1".
		Then a random thread calls getTextureId for a texture called
		"stone.png^mineral1^crack0".
		...Now, WTF should happen? Well:
		- getTextureId strips off stuff recursively from the end until
		  the remaining part is found, or nothing is left when
		  something is stripped out

		But it is slow to search for textures by names and modify them
		like that?
		- ContentFeatures is made to contain ids for the basic plain
		  textures
		- Crack textures can be slow by themselves, but the framework
		  must be fast.

		Example case #2:
		- Assume a texture with the id 1 exists, and has the name
		  "stone.png^mineral1" and is specified as a part of some atlas.
		- Now MapBlock::getNodeTile() stumbles upon a node which uses
		  texture id 1, and finds out that NODEMOD_CRACK must be applied
		  with progression=0
		- It finds out the name of the texture with getTextureName(1),
		  appends "^crack0" to it and gets a new texture id with
		  getTextureId("stone.png^mineral1^crack0")

	*/
	
	/*
		Gets a texture id from cache or
		- if main thread, from getTextureIdDirect
		- if other thread, adds to request queue and waits for main thread
	*/
	u32 getTextureId(const std::string &name);
	
	/*
		Example names:
		"stone.png"
		"stone.png^crack2"
		"stone.png^blit:mineral_coal.png"
		"stone.png^blit:mineral_coal.png^crack1"

		- If texture specified by name is found from cache, return the
		  cached id.
		- Otherwise generate the texture, add to cache and return id.
		  Recursion is used to find out the largest found part of the
		  texture and continue based on it.

		The id 0 points to a NULL texture. It is returned in case of error.
	*/
	u32 getTextureIdDirect(const std::string &name);

	/*
		Finds out the name of a cached texture.
	*/
	std::string getTextureName(u32 id);

	/*
		If texture specified by the name pointed by the id doesn't
		exist, create it, then return the cached texture.

		Can be called from any thread. If called from some other thread
		and not found in cache, the call is queued to the main thread
		for processing.
	*/
	AtlasPointer getTexture(u32 id);
	
	AtlasPointer getTexture(const std::string &name)
	{
		return getTexture(getTextureId(name));
	}
	
	// Gets a separate texture
	video::ITexture* getTextureRaw(const std::string &name)
	{
		AtlasPointer ap = getTexture(name);
		return ap.atlas;
	}

private:
	/*
		Build the main texture atlas which contains most of the
		textures.
		
		This is called by the constructor.
	*/
	void buildMainAtlas();
	
	// The id of the thread that is allowed to use irrlicht directly
	threadid_t m_main_thread;
	// The irrlicht device
	IrrlichtDevice *m_device;
	
	// A texture id is index in this array.
	// The first position contains a NULL texture.
	core::array<SourceAtlasPointer> m_atlaspointer_cache;
	// Maps a texture name to an index in the former.
	core::map<std::string, u32> m_name_to_id;
	// The two former containers are behind this mutex
	JMutex m_atlaspointer_cache_mutex;
	
	// Main texture atlas. This is filled at startup and is then not touched.
	video::IImage *m_main_atlas_image;
	video::ITexture *m_main_atlas_texture;

	// Queued texture fetches (to be processed by the main thread)
	RequestQueue<std::string, u32, u8, u8> m_get_texture_queue;
};

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
		material_type(MATERIAL_ALPHA_NONE),
		// Use this so that leaves don't need a separate material
		//material_type(MATERIAL_ALPHA_SIMPLE),
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

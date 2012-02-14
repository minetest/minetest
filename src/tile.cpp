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

#include "tile.h"
#include "debug.h"
#include "main.h" // for g_settings
#include "filesys.h"
#include "utility.h"
#include "settings.h"
#include "mesh.h"
#include <ICameraSceneNode.h>
#include "log.h"
#include "mapnode.h" // For texture atlas making
#include "nodedef.h" // For texture atlas making
#include "gamedef.h"

/*
	A cache from texture name to texture path
*/
MutexedMap<std::string, std::string> g_texturename_to_path_cache;

/*
	Replaces the filename extension.
	eg:
		std::string image = "a/image.png"
		replace_ext(image, "jpg")
		-> image = "a/image.jpg"
	Returns true on success.
*/
static bool replace_ext(std::string &path, const char *ext)
{
	if(ext == NULL)
		return false;
	// Find place of last dot, fail if \ or / found.
	s32 last_dot_i = -1;
	for(s32 i=path.size()-1; i>=0; i--)
	{
		if(path[i] == '.')
		{
			last_dot_i = i;
			break;
		}
		
		if(path[i] == '\\' || path[i] == '/')
			break;
	}
	// If not found, return an empty string
	if(last_dot_i == -1)
		return false;
	// Else make the new path
	path = path.substr(0, last_dot_i+1) + ext;
	return true;
}

/*
	Find out the full path of an image by trying different filename
	extensions.

	If failed, return "".
*/
static std::string getImagePath(std::string path)
{
	// A NULL-ended list of possible image extensions
	const char *extensions[] = {
		"png", "jpg", "bmp", "tga",
		"pcx", "ppm", "psd", "wal", "rgb",
		NULL
	};

	const char **ext = extensions;
	do{
		bool r = replace_ext(path, *ext);
		if(r == false)
			return "";
		if(fs::PathExists(path))
			return path;
	}
	while((++ext) != NULL);
	
	return "";
}

/*
	Gets the path to a texture by first checking if the texture exists
	in texture_path and if not, using the data path.

	Checks all supported extensions by replacing the original extension.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getTexturePath(const std::string &filename)
{
	std::string fullpath = "";
	/*
		Check from cache
	*/
	bool incache = g_texturename_to_path_cache.get(filename, &fullpath);
	if(incache)
		return fullpath;
	
	/*
		Check from texture_path
	*/
	std::string texture_path = g_settings->get("texture_path");
	if(texture_path != "")
	{
		std::string testpath = texture_path + DIR_DELIM + filename;
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
	}
	
	/*
		Check from default data directory
	*/
	if(fullpath == "")
	{
		std::string rel_path = std::string("clienttextures")+DIR_DELIM+filename;
		std::string testpath = porting::path_data + DIR_DELIM + rel_path;
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
	}
	
	// Add to cache (also an empty result is cached)
	g_texturename_to_path_cache.set(filename, fullpath);
	
	// Finally return it
	return fullpath;
}

/*
	An internal variant of AtlasPointer with more data.
	(well, more like a wrapper)
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
	SourceImageCache: A cache used for storing source images.
*/

class SourceImageCache
{
public:
	void insert(const std::string &name, video::IImage *img,
			bool prefer_local, video::IVideoDriver *driver)
	{
		assert(img);
		// Remove old image
		core::map<std::string, video::IImage*>::Node *n;
		n = m_images.find(name);
		if(n){
			video::IImage *oldimg = n->getValue();
			if(oldimg)
				oldimg->drop();
		}
		// Try to use local texture instead if asked to
		if(prefer_local){
			std::string path = getTexturePath(name.c_str());
			if(path != ""){
				video::IImage *img2 = driver->createImageFromFile(path.c_str());
				if(img2){
					m_images[name] = img2;
					return;
				}
			}
		}
		img->grab();
		m_images[name] = img;
	}
	video::IImage* get(const std::string &name)
	{
		core::map<std::string, video::IImage*>::Node *n;
		n = m_images.find(name);
		if(n)
			return n->getValue();
		return NULL;
	}
	// Primarily fetches from cache, secondarily tries to read from filesystem
	video::IImage* getOrLoad(const std::string &name, IrrlichtDevice *device)
	{
		core::map<std::string, video::IImage*>::Node *n;
		n = m_images.find(name);
		if(n){
			n->getValue()->grab(); // Grab for caller
			return n->getValue();
		}
		video::IVideoDriver* driver = device->getVideoDriver();
		std::string path = getTexturePath(name.c_str());
		if(path == ""){
			infostream<<"SourceImageCache::getOrLoad(): No path found for \""
					<<name<<"\""<<std::endl;
			return NULL;
		}
		infostream<<"SourceImageCache::getOrLoad(): Loading path \""<<path
				<<"\""<<std::endl;
		video::IImage *img = driver->createImageFromFile(path.c_str());
		// Even if could not be loaded, put as NULL
		//m_images[name] = img;
		if(img){
			m_images[name] = img;
			img->grab(); // Grab for caller
		}
		return img;
	}
private:
	core::map<std::string, video::IImage*> m_images;
};

/*
	TextureSource
*/

class TextureSource : public IWritableTextureSource
{
public:
	TextureSource(IrrlichtDevice *device);
	~TextureSource();

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
		"stone.png^mineral_coal.png"
		"stone.png^mineral_coal.png^crack1"

		- If texture specified by name is found from cache, return the
		  cached id.
		- Otherwise generate the texture, add to cache and return id.
		  Recursion is used to find out the largest found part of the
		  texture and continue based on it.

		The id 0 points to a NULL texture. It is returned in case of error.
	*/
	u32 getTextureIdDirect(const std::string &name);

	// Finds out the name of a cached texture.
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
		AtlasPointer ap = getTexture(name + "^[forcesingle");
		return ap.atlas;
	}

	// Returns a pointer to the irrlicht device
	virtual IrrlichtDevice* getDevice()
	{
		return m_device;
	}

	// Update new texture pointer and texture coordinates to an
	// AtlasPointer based on it's texture id
	void updateAP(AtlasPointer &ap);

	// Processes queued texture requests from other threads.
	// Shall be called from the main thread.
	void processQueue();
	
	// Insert an image into the cache without touching the filesystem.
	// Shall be called from the main thread.
	void insertSourceImage(const std::string &name, video::IImage *img);
	
	// Rebuild images and textures from the current set of source images
	// Shall be called from the main thread.
	void rebuildImagesAndTextures();

	// Build the main texture atlas which contains most of the
	// textures.
	void buildMainAtlas(class IGameDef *gamedef);
	
private:
	
	// The id of the thread that is allowed to use irrlicht directly
	threadid_t m_main_thread;
	// The irrlicht device
	IrrlichtDevice *m_device;
	
	// Cache of source images
	// This should be only accessed from the main thread
	SourceImageCache m_sourcecache;

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

IWritableTextureSource* createTextureSource(IrrlichtDevice *device)
{
	return new TextureSource(device);
}

TextureSource::TextureSource(IrrlichtDevice *device):
		m_device(device),
		m_main_atlas_image(NULL),
		m_main_atlas_texture(NULL)
{
	assert(m_device);
	
	m_atlaspointer_cache_mutex.Init();
	
	m_main_thread = get_current_thread_id();
	
	// Add a NULL AtlasPointer as the first index, named ""
	m_atlaspointer_cache.push_back(SourceAtlasPointer(""));
	m_name_to_id[""] = 0;
}

TextureSource::~TextureSource()
{
}

u32 TextureSource::getTextureId(const std::string &name)
{
	//infostream<<"getTextureId(): \""<<name<<"\""<<std::endl;

	{
		/*
			See if texture already exists
		*/
		JMutexAutoLock lock(m_atlaspointer_cache_mutex);
		core::map<std::string, u32>::Node *n;
		n = m_name_to_id.find(name);
		if(n != NULL)
		{
			return n->getValue();
		}
	}
	
	/*
		Get texture
	*/
	if(get_current_thread_id() == m_main_thread)
	{
		return getTextureIdDirect(name);
	}
	else
	{
		infostream<<"getTextureId(): Queued: name=\""<<name<<"\""<<std::endl;

		// We're gonna ask the result to be put into here
		ResultQueue<std::string, u32, u8, u8> result_queue;
		
		// Throw a request in
		m_get_texture_queue.add(name, 0, 0, &result_queue);
		
		infostream<<"Waiting for texture from main thread, name=\""
				<<name<<"\""<<std::endl;
		
		try
		{
			// Wait result for a second
			GetResult<std::string, u32, u8, u8>
					result = result_queue.pop_front(1000);
		
			// Check that at least something worked OK
			assert(result.key == name);

			return result.item;
		}
		catch(ItemNotFoundException &e)
		{
			infostream<<"Waiting for texture timed out."<<std::endl;
			return 0;
		}
	}
	
	infostream<<"getTextureId(): Failed"<<std::endl;

	return 0;
}

// Brighten image
void brighten(video::IImage *image);

/*
	Generate image based on a string like "stone.png" or "[crack0".
	if baseimg is NULL, it is created. Otherwise stuff is made on it.
*/
bool generate_image(std::string part_of_name, video::IImage *& baseimg,
		IrrlichtDevice *device, SourceImageCache *sourcecache);

/*
	Generates an image from a full string like
	"stone.png^mineral_coal.png^[crack0".

	This is used by buildMainAtlas().
*/
video::IImage* generate_image_from_scratch(std::string name,
		IrrlichtDevice *device, SourceImageCache *sourcecache);

/*
	This method generates all the textures
*/
u32 TextureSource::getTextureIdDirect(const std::string &name)
{
	//infostream<<"getTextureIdDirect(): name=\""<<name<<"\""<<std::endl;

	// Empty name means texture 0
	if(name == "")
	{
		infostream<<"getTextureIdDirect(): name is empty"<<std::endl;
		return 0;
	}
	
	/*
		Calling only allowed from main thread
	*/
	if(get_current_thread_id() != m_main_thread)
	{
		errorstream<<"TextureSource::getTextureIdDirect() "
				"called not from main thread"<<std::endl;
		return 0;
	}

	/*
		See if texture already exists
	*/
	{
		JMutexAutoLock lock(m_atlaspointer_cache_mutex);

		core::map<std::string, u32>::Node *n;
		n = m_name_to_id.find(name);
		if(n != NULL)
		{
			/*infostream<<"getTextureIdDirect(): \""<<name
					<<"\" found in cache"<<std::endl;*/
			return n->getValue();
		}
	}

	/*infostream<<"getTextureIdDirect(): \""<<name
			<<"\" NOT found in cache. Creating it."<<std::endl;*/
	
	/*
		Get the base image
	*/

	char separator = '^';

	/*
		This is set to the id of the base image.
		If left 0, there is no base image and a completely new image
		is made.
	*/
	u32 base_image_id = 0;
	
	// Find last meta separator in name
	s32 last_separator_position = -1;
	for(s32 i=name.size()-1; i>=0; i--)
	{
		if(name[i] == separator)
		{
			last_separator_position = i;
			break;
		}
	}
	/*
		If separator was found, construct the base name and make the
		base image using a recursive call
	*/
	std::string base_image_name;
	if(last_separator_position != -1)
	{
		// Construct base name
		base_image_name = name.substr(0, last_separator_position);
		/*infostream<<"getTextureIdDirect(): Calling itself recursively"
				" to get base image of \""<<name<<"\" = \""
                <<base_image_name<<"\""<<std::endl;*/
		base_image_id = getTextureIdDirect(base_image_name);
	}
	
	//infostream<<"base_image_id="<<base_image_id<<std::endl;
	
	video::IVideoDriver* driver = m_device->getVideoDriver();
	assert(driver);

	video::ITexture *t = NULL;

	/*
		An image will be built from files and then converted into a texture.
	*/
	video::IImage *baseimg = NULL;
	
	// If a base image was found, copy it to baseimg
	if(base_image_id != 0)
	{
		JMutexAutoLock lock(m_atlaspointer_cache_mutex);

		SourceAtlasPointer ap = m_atlaspointer_cache[base_image_id];

		video::IImage *image = ap.atlas_img;
		
		if(image == NULL)
		{
			infostream<<"getTextureIdDirect(): WARNING: NULL image in "
					<<"cache: \""<<base_image_name<<"\""
					<<std::endl;
		}
		else
		{
			core::dimension2d<u32> dim = ap.intsize;

			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);

			core::position2d<s32> pos_to(0,0);
			core::position2d<s32> pos_from = ap.intpos;
			
			image->copyTo(
					baseimg, // target
					v2s32(0,0), // position in target
					core::rect<s32>(pos_from, dim) // from
			);

			/*infostream<<"getTextureIdDirect(): Loaded \""
					<<base_image_name<<"\" from image cache"
					<<std::endl;*/
		}
	}
	
	/*
		Parse out the last part of the name of the image and act
		according to it
	*/

	std::string last_part_of_name = name.substr(last_separator_position+1);
	//infostream<<"last_part_of_name=\""<<last_part_of_name<<"\""<<std::endl;

	// Generate image according to part of name
	if(!generate_image(last_part_of_name, baseimg, m_device, &m_sourcecache))
	{
		errorstream<<"getTextureIdDirect(): "
				"failed to generate \""<<last_part_of_name<<"\""
				<<std::endl;
	}

	// If no resulting image, print a warning
	if(baseimg == NULL)
	{
		errorstream<<"getTextureIdDirect(): baseimg is NULL (attempted to"
				" create texture \""<<name<<"\""<<std::endl;
	}
	
	if(baseimg != NULL)
	{
		// Create texture from resulting image
		t = driver->addTexture(name.c_str(), baseimg);
	}
	
	/*
		Add texture to caches (add NULL textures too)
	*/

	JMutexAutoLock lock(m_atlaspointer_cache_mutex);
	
	u32 id = m_atlaspointer_cache.size();
	AtlasPointer ap(id);
	ap.atlas = t;
	ap.pos = v2f(0,0);
	ap.size = v2f(1,1);
	ap.tiled = 0;
	core::dimension2d<u32> baseimg_dim(0,0);
	if(baseimg)
		baseimg_dim = baseimg->getDimension();
	SourceAtlasPointer nap(name, ap, baseimg, v2s32(0,0), baseimg_dim);
	m_atlaspointer_cache.push_back(nap);
	m_name_to_id.insert(name, id);

	/*infostream<<"getTextureIdDirect(): "
			<<"Returning id="<<id<<" for name \""<<name<<"\""<<std::endl;*/
	
	return id;
}

std::string TextureSource::getTextureName(u32 id)
{
	JMutexAutoLock lock(m_atlaspointer_cache_mutex);

	if(id >= m_atlaspointer_cache.size())
	{
		errorstream<<"TextureSource::getTextureName(): id="<<id
				<<" >= m_atlaspointer_cache.size()="
				<<m_atlaspointer_cache.size()<<std::endl;
		return "";
	}
	
	return m_atlaspointer_cache[id].name;
}


AtlasPointer TextureSource::getTexture(u32 id)
{
	JMutexAutoLock lock(m_atlaspointer_cache_mutex);

	if(id >= m_atlaspointer_cache.size())
		return AtlasPointer(0, NULL);
	
	return m_atlaspointer_cache[id].a;
}

void TextureSource::updateAP(AtlasPointer &ap)
{
	AtlasPointer ap2 = getTexture(ap.id);
	ap = ap2;
}

void TextureSource::processQueue()
{
	/*
		Fetch textures
	*/
	if(m_get_texture_queue.size() > 0)
	{
		GetRequest<std::string, u32, u8, u8>
				request = m_get_texture_queue.pop();

		/*infostream<<"TextureSource::processQueue(): "
				<<"got texture request with "
				<<"name=\""<<request.key<<"\""
				<<std::endl;*/

		GetResult<std::string, u32, u8, u8>
				result;
		result.key = request.key;
		result.callers = request.callers;
		result.item = getTextureIdDirect(request.key);

		request.dest->push_back(result);
	}
}

void TextureSource::insertSourceImage(const std::string &name, video::IImage *img)
{
	//infostream<<"TextureSource::insertSourceImage(): name="<<name<<std::endl;
	
	assert(get_current_thread_id() == m_main_thread);
	
	m_sourcecache.insert(name, img, true, m_device->getVideoDriver());
}
	
void TextureSource::rebuildImagesAndTextures()
{
	JMutexAutoLock lock(m_atlaspointer_cache_mutex);

	/*// Oh well... just clear everything, they'll load sometime.
	m_atlaspointer_cache.clear();
	m_name_to_id.clear();*/

	video::IVideoDriver* driver = m_device->getVideoDriver();
	
	// Remove source images from textures to disable inheriting textures
	// from existing textures
	/*for(u32 i=0; i<m_atlaspointer_cache.size(); i++){
		SourceAtlasPointer *sap = &m_atlaspointer_cache[i];
		sap->atlas_img->drop();
		sap->atlas_img = NULL;
	}*/
	
	// Recreate textures
	for(u32 i=0; i<m_atlaspointer_cache.size(); i++){
		SourceAtlasPointer *sap = &m_atlaspointer_cache[i];
		video::IImage *img =
			generate_image_from_scratch(sap->name, m_device, &m_sourcecache);
		// Create texture from resulting image
		video::ITexture *t = NULL;
		if(img)
			t = driver->addTexture(sap->name.c_str(), img);
		
		// Replace texture
		sap->a.atlas = t;
		sap->a.pos = v2f(0,0);
		sap->a.size = v2f(1,1);
		sap->a.tiled = 0;
		sap->atlas_img = img;
		sap->intpos = v2s32(0,0);
		sap->intsize = img->getDimension();
	}
}

void TextureSource::buildMainAtlas(class IGameDef *gamedef) 
{
	assert(gamedef->tsrc() == this);
	INodeDefManager *ndef = gamedef->ndef();

	infostream<<"TextureSource::buildMainAtlas()"<<std::endl;

	//return; // Disable (for testing)
	
	video::IVideoDriver* driver = m_device->getVideoDriver();
	assert(driver);

	JMutexAutoLock lock(m_atlaspointer_cache_mutex);

	// Create an image of the right size
	core::dimension2d<u32> atlas_dim(1024,1024);
	video::IImage *atlas_img =
			driver->createImage(video::ECF_A8R8G8B8, atlas_dim);
	//assert(atlas_img);
	if(atlas_img == NULL)
	{
		errorstream<<"TextureSource::buildMainAtlas(): Failed to create atlas "
				"image; not building texture atlas."<<std::endl;
		return;
	}

	/*
		Grab list of stuff to include in the texture atlas from the
		main content features
	*/

	core::map<std::string, bool> sourcelist;

	for(u16 j=0; j<MAX_CONTENT+1; j++)
	{
		if(j == CONTENT_IGNORE || j == CONTENT_AIR)
			continue;
		const ContentFeatures &f = ndef->get(j);
		for(u32 i=0; i<6; i++)
		{
			std::string name = f.tname_tiles[i];
			sourcelist[name] = true;
		}
	}
	
	infostream<<"Creating texture atlas out of textures: ";
	for(core::map<std::string, bool>::Iterator
			i = sourcelist.getIterator();
			i.atEnd() == false; i++)
	{
		std::string name = i.getNode()->getKey();
		infostream<<"\""<<name<<"\" ";
	}
	infostream<<std::endl;

	// Padding to disallow texture bleeding
	s32 padding = 16;

	s32 column_width = 256;
	s32 column_padding = 16;

	/*
		First pass: generate almost everything
	*/
	core::position2d<s32> pos_in_atlas(0,0);
	
	pos_in_atlas.Y = padding;

	for(core::map<std::string, bool>::Iterator
			i = sourcelist.getIterator();
			i.atEnd() == false; i++)
	{
		std::string name = i.getNode()->getKey();

		// Generate image by name
		video::IImage *img2 = generate_image_from_scratch(name, m_device,
				&m_sourcecache);
		if(img2 == NULL)
		{
			errorstream<<"TextureSource::buildMainAtlas(): "
					<<"Couldn't generate image \""<<name<<"\""<<std::endl;
			continue;
		}

		core::dimension2d<u32> dim = img2->getDimension();

		// Don't add to atlas if image is large
		core::dimension2d<u32> max_size_in_atlas(32,32);
		if(dim.Width > max_size_in_atlas.Width
		|| dim.Height > max_size_in_atlas.Height)
		{
			infostream<<"TextureSource::buildMainAtlas(): Not adding "
					<<"\""<<name<<"\" because image is large"<<std::endl;
			continue;
		}

		// Wrap columns and stop making atlas if atlas is full
		if(pos_in_atlas.Y + dim.Height > atlas_dim.Height)
		{
			if(pos_in_atlas.X > (s32)atlas_dim.Width - 256 - padding){
				errorstream<<"TextureSource::buildMainAtlas(): "
						<<"Atlas is full, not adding more textures."
						<<std::endl;
				break;
			}
			pos_in_atlas.Y = padding;
			pos_in_atlas.X += column_width + column_padding;
		}
		
		/*infostream<<"TextureSource::buildMainAtlas(): Adding \""<<name
				<<"\" to texture atlas"<<std::endl;*/

		// Tile it a few times in the X direction
		u16 xwise_tiling = column_width / dim.Width;
		if(xwise_tiling > 16) // Limit to 16 (more gives no benefit)
			xwise_tiling = 16;
		for(u32 j=0; j<xwise_tiling; j++)
		{
			// Copy the copy to the atlas
			/*img2->copyToWithAlpha(atlas_img,
					pos_in_atlas + v2s32(j*dim.Width,0),
					core::rect<s32>(v2s32(0,0), dim),
					video::SColor(255,255,255,255),
					NULL);*/
			img2->copyTo(atlas_img,
					pos_in_atlas + v2s32(j*dim.Width,0),
					core::rect<s32>(v2s32(0,0), dim),
					NULL);
		}

		// Copy the borders a few times to disallow texture bleeding
		for(u32 side=0; side<2; side++) // top and bottom
		for(s32 y0=0; y0<padding; y0++)
		for(s32 x0=0; x0<(s32)xwise_tiling*(s32)dim.Width; x0++)
		{
			s32 dst_y;
			s32 src_y;
			if(side==0)
			{
				dst_y = y0 + pos_in_atlas.Y + dim.Height;
				src_y = pos_in_atlas.Y + dim.Height - 1;
			}
			else
			{
				dst_y = -y0 + pos_in_atlas.Y-1;
				src_y = pos_in_atlas.Y;
			}
			s32 x = x0 + pos_in_atlas.X;
			video::SColor c = atlas_img->getPixel(x, src_y);
			atlas_img->setPixel(x,dst_y,c);
		}

		img2->drop();

		/*
			Add texture to caches
		*/
		
		bool reuse_old_id = false;
		u32 id = m_atlaspointer_cache.size();
		// Check old id without fetching a texture
		core::map<std::string, u32>::Node *n;
		n = m_name_to_id.find(name);
		// If it exists, we will replace the old definition
		if(n){
			id = n->getValue();
			reuse_old_id = true;
			/*infostream<<"TextureSource::buildMainAtlas(): "
					<<"Replacing old AtlasPointer"<<std::endl;*/
		}

		// Create AtlasPointer
		AtlasPointer ap(id);
		ap.atlas = NULL; // Set on the second pass
		ap.pos = v2f((float)pos_in_atlas.X/(float)atlas_dim.Width,
				(float)pos_in_atlas.Y/(float)atlas_dim.Height);
		ap.size = v2f((float)dim.Width/(float)atlas_dim.Width,
				(float)dim.Width/(float)atlas_dim.Height);
		ap.tiled = xwise_tiling;

		// Create SourceAtlasPointer and add to containers
		SourceAtlasPointer nap(name, ap, atlas_img, pos_in_atlas, dim);
		if(reuse_old_id)
			m_atlaspointer_cache[id] = nap;
		else
			m_atlaspointer_cache.push_back(nap);
		m_name_to_id[name] = id;
			
		// Increment position
		pos_in_atlas.Y += dim.Height + padding * 2;
	}

	/*
		Make texture
	*/
	video::ITexture *t = driver->addTexture("__main_atlas__", atlas_img);
	assert(t);

	/*
		Second pass: set texture pointer in generated AtlasPointers
	*/
	for(core::map<std::string, bool>::Iterator
			i = sourcelist.getIterator();
			i.atEnd() == false; i++)
	{
		std::string name = i.getNode()->getKey();
		if(m_name_to_id.find(name) == NULL)
			continue;
		u32 id = m_name_to_id[name];
		//infostream<<"id of name "<<name<<" is "<<id<<std::endl;
		m_atlaspointer_cache[id].a.atlas = t;
	}

	/*
		Write image to file so that it can be inspected
	*/
	/*std::string atlaspath = porting::path_userdata
			+ DIR_DELIM + "generated_texture_atlas.png";
	infostream<<"Removing and writing texture atlas for inspection to "
			<<atlaspath<<std::endl;
	fs::RecursiveDelete(atlaspath);
	driver->writeImageToFile(atlas_img, atlaspath.c_str());*/
}

video::IImage* generate_image_from_scratch(std::string name,
		IrrlichtDevice *device, SourceImageCache *sourcecache)
{
	/*infostream<<"generate_image_from_scratch(): "
			"\""<<name<<"\""<<std::endl;*/
	
	video::IVideoDriver* driver = device->getVideoDriver();
	assert(driver);

	/*
		Get the base image
	*/

	video::IImage *baseimg = NULL;

	char separator = '^';

	// Find last meta separator in name
	s32 last_separator_position = -1;
	for(s32 i=name.size()-1; i>=0; i--)
	{
		if(name[i] == separator)
		{
			last_separator_position = i;
			break;
		}
	}

	/*infostream<<"generate_image_from_scratch(): "
			<<"last_separator_position="<<last_separator_position
			<<std::endl;*/

	/*
		If separator was found, construct the base name and make the
		base image using a recursive call
	*/
	std::string base_image_name;
	if(last_separator_position != -1)
	{
		// Construct base name
		base_image_name = name.substr(0, last_separator_position);
		/*infostream<<"generate_image_from_scratch(): Calling itself recursively"
				" to get base image of \""<<name<<"\" = \""
                <<base_image_name<<"\""<<std::endl;*/
		baseimg = generate_image_from_scratch(base_image_name, device,
				sourcecache);
	}
	
	/*
		Parse out the last part of the name of the image and act
		according to it
	*/

	std::string last_part_of_name = name.substr(last_separator_position+1);
	//infostream<<"last_part_of_name=\""<<last_part_of_name<<"\""<<std::endl;
	
	// Generate image according to part of name
	if(!generate_image(last_part_of_name, baseimg, device, sourcecache))
	{
		errorstream<<"generate_image_from_scratch(): "
				"failed to generate \""<<last_part_of_name<<"\""
				<<std::endl;
		return NULL;
	}
	
	return baseimg;
}

bool generate_image(std::string part_of_name, video::IImage *& baseimg,
		IrrlichtDevice *device, SourceImageCache *sourcecache)
{
	video::IVideoDriver* driver = device->getVideoDriver();
	assert(driver);

	// Stuff starting with [ are special commands
	if(part_of_name.size() == 0 || part_of_name[0] != '[')
	{
		video::IImage *image = sourcecache->getOrLoad(part_of_name, device);

		if(image == NULL)
		{
			if(part_of_name != ""){
				errorstream<<"generate_image(): Could not load image \""
						<<part_of_name<<"\""<<" while building texture"<<std::endl;
				errorstream<<"generate_image(): Creating a dummy"
						<<" image for \""<<part_of_name<<"\""<<std::endl;
			}

			// Just create a dummy image
			//core::dimension2d<u32> dim(2,2);
			core::dimension2d<u32> dim(1,1);
			image = driver->createImage(video::ECF_A8R8G8B8, dim);
			assert(image);
			/*image->setPixel(0,0, video::SColor(255,255,0,0));
			image->setPixel(1,0, video::SColor(255,0,255,0));
			image->setPixel(0,1, video::SColor(255,0,0,255));
			image->setPixel(1,1, video::SColor(255,255,0,255));*/
			image->setPixel(0,0, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));
			/*image->setPixel(1,0, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));
			image->setPixel(0,1, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));
			image->setPixel(1,1, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));*/
		}

		// If base image is NULL, load as base.
		if(baseimg == NULL)
		{
			//infostream<<"Setting "<<part_of_name<<" as base"<<std::endl;
			/*
				Copy it this way to get an alpha channel.
				Otherwise images with alpha cannot be blitted on 
				images that don't have alpha in the original file.
			*/
			core::dimension2d<u32> dim = image->getDimension();
			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
			image->copyTo(baseimg);
			image->drop();
		}
		// Else blit on base.
		else
		{
			//infostream<<"Blitting "<<part_of_name<<" on base"<<std::endl;
			// Size of the copied area
			core::dimension2d<u32> dim = image->getDimension();
			//core::dimension2d<u32> dim(16,16);
			// Position to copy the blitted to in the base image
			core::position2d<s32> pos_to(0,0);
			// Position to copy the blitted from in the blitted image
			core::position2d<s32> pos_from(0,0);
			// Blit
			image->copyToWithAlpha(baseimg, pos_to,
					core::rect<s32>(pos_from, dim),
					video::SColor(255,255,255,255),
					NULL);
			// Drop image
			image->drop();
		}
	}
	else
	{
		// A special texture modification

		/*infostream<<"generate_image(): generating special "
				<<"modification \""<<part_of_name<<"\""
				<<std::endl;*/
		
		/*
			This is the simplest of all; it just adds stuff to the
			name so that a separate texture is created.

			It is used to make textures for stuff that doesn't want
			to implement getting the texture from a bigger texture
			atlas.
		*/
		if(part_of_name == "[forcesingle")
		{
			// If base image is NULL, create a random color
			if(baseimg == NULL)
			{
				core::dimension2d<u32> dim(1,1);
				baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
				assert(baseimg);
				baseimg->setPixel(0,0, video::SColor(255,myrand()%256,
						myrand()%256,myrand()%256));
			}
		}
		/*
			[crackN
			Adds a cracking texture
		*/
		else if(part_of_name.substr(0,6) == "[crack")
		{
			if(baseimg == NULL)
			{
				errorstream<<"generate_image(): baseimg==NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}
			
			// Crack image number
			u16 progression = stoi(part_of_name.substr(6));

			// Size of the base image
			core::dimension2d<u32> dim_base = baseimg->getDimension();
			
			/*
				Load crack image.

				It is an image with a number of cracking stages
				horizontally tiled.
			*/
			video::IImage *img_crack = sourcecache->getOrLoad("crack.png", device);
		
			if(img_crack)
			{
				// Dimension of original image
				core::dimension2d<u32> dim_crack
						= img_crack->getDimension();
				// Count of crack stages
				u32 crack_count = dim_crack.Height / dim_crack.Width;
				// Limit progression
				if(progression > crack_count-1)
					progression = crack_count-1;
				// Dimension of a single scaled crack stage
				core::dimension2d<u32> dim_crack_scaled_single(
					dim_base.Width,
					dim_base.Height
				);
				// Dimension of scaled size
				core::dimension2d<u32> dim_crack_scaled(
					dim_crack_scaled_single.Width,
					dim_crack_scaled_single.Height * crack_count
				);
				// Create scaled crack image
				video::IImage *img_crack_scaled = driver->createImage(
						video::ECF_A8R8G8B8, dim_crack_scaled);
				if(img_crack_scaled)
				{
					// Scale crack image by copying
					img_crack->copyToScaling(img_crack_scaled);
					
					// Position to copy the crack from
					core::position2d<s32> pos_crack_scaled(
						0,
						dim_crack_scaled_single.Height * progression
					);
					
					// This tiling does nothing currently but is useful
					for(u32 y0=0; y0<dim_base.Height
							/ dim_crack_scaled_single.Height; y0++)
					for(u32 x0=0; x0<dim_base.Width
							/ dim_crack_scaled_single.Width; x0++)
					{
						// Position to copy the crack to in the base image
						core::position2d<s32> pos_base(
							x0*dim_crack_scaled_single.Width,
							y0*dim_crack_scaled_single.Height
						);
						// Rectangle to copy the crack from on the scaled image
						core::rect<s32> rect_crack_scaled(
							pos_crack_scaled,
							dim_crack_scaled_single
						);
						// Copy it
						img_crack_scaled->copyToWithAlpha(baseimg, pos_base,
								rect_crack_scaled,
								video::SColor(255,255,255,255),
								NULL);
					}

					img_crack_scaled->drop();
				}
				
				img_crack->drop();
			}
		}
		/*
			[combine:WxH:X,Y=filename:X,Y=filename2
			Creates a bigger texture from an amount of smaller ones
		*/
		else if(part_of_name.substr(0,8) == "[combine")
		{
			Strfnd sf(part_of_name);
			sf.next(":");
			u32 w0 = stoi(sf.next("x"));
			u32 h0 = stoi(sf.next(":"));
			infostream<<"combined w="<<w0<<" h="<<h0<<std::endl;
			core::dimension2d<u32> dim(w0,h0);
			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
			while(sf.atend() == false)
			{
				u32 x = stoi(sf.next(","));
				u32 y = stoi(sf.next("="));
				std::string filename = sf.next(":");
				infostream<<"Adding \""<<filename
						<<"\" to combined ("<<x<<","<<y<<")"
						<<std::endl;
				video::IImage *img = sourcecache->getOrLoad(filename, device);
				if(img)
				{
					core::dimension2d<u32> dim = img->getDimension();
					infostream<<"Size "<<dim.Width
							<<"x"<<dim.Height<<std::endl;
					core::position2d<s32> pos_base(x, y);
					video::IImage *img2 =
							driver->createImage(video::ECF_A8R8G8B8, dim);
					img->copyTo(img2);
					img->drop();
					img2->copyToWithAlpha(baseimg, pos_base,
							core::rect<s32>(v2s32(0,0), dim),
							video::SColor(255,255,255,255),
							NULL);
					img2->drop();
				}
				else
				{
					infostream<<"img==NULL"<<std::endl;
				}
			}
		}
		/*
			"[brighten"
		*/
		else if(part_of_name.substr(0,9) == "[brighten")
		{
			if(baseimg == NULL)
			{
				errorstream<<"generate_image(): baseimg==NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			brighten(baseimg);
		}
		/*
			"[noalpha"
			Make image completely opaque.
			Used for the leaves texture when in old leaves mode, so
			that the transparent parts don't look completely black 
			when simple alpha channel is used for rendering.
		*/
		else if(part_of_name.substr(0,8) == "[noalpha")
		{
			if(baseimg == NULL)
			{
				errorstream<<"generate_image(): baseimg==NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			core::dimension2d<u32> dim = baseimg->getDimension();
			
			// Set alpha to full
			for(u32 y=0; y<dim.Height; y++)
			for(u32 x=0; x<dim.Width; x++)
			{
				video::SColor c = baseimg->getPixel(x,y);
				c.setAlpha(255);
				baseimg->setPixel(x,y,c);
			}
		}
		/*
			"[makealpha:R,G,B"
			Convert one color to transparent.
		*/
		else if(part_of_name.substr(0,11) == "[makealpha:")
		{
			if(baseimg == NULL)
			{
				errorstream<<"generate_image(): baseimg==NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			Strfnd sf(part_of_name.substr(11));
			u32 r1 = stoi(sf.next(","));
			u32 g1 = stoi(sf.next(","));
			u32 b1 = stoi(sf.next(""));
			std::string filename = sf.next("");

			core::dimension2d<u32> dim = baseimg->getDimension();
			
			/*video::IImage *oldbaseimg = baseimg;
			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
			oldbaseimg->copyTo(baseimg);
			oldbaseimg->drop();*/

			// Set alpha to full
			for(u32 y=0; y<dim.Height; y++)
			for(u32 x=0; x<dim.Width; x++)
			{
				video::SColor c = baseimg->getPixel(x,y);
				u32 r = c.getRed();
				u32 g = c.getGreen();
				u32 b = c.getBlue();
				if(!(r == r1 && g == g1 && b == b1))
					continue;
				c.setAlpha(0);
				baseimg->setPixel(x,y,c);
			}
		}
		/*
			[inventorycube{topimage{leftimage{rightimage
			In every subimage, replace ^ with &.
			Create an "inventory cube".
			NOTE: This should be used only on its own.
			Example (a grass block (not actually used in game):
			"[inventorycube{grass.png{mud.png&grass_side.png{mud.png&grass_side.png"
		*/
		else if(part_of_name.substr(0,14) == "[inventorycube")
		{
			if(baseimg != NULL)
			{
				errorstream<<"generate_image(): baseimg!=NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			str_replace_char(part_of_name, '&', '^');
			Strfnd sf(part_of_name);
			sf.next("{");
			std::string imagename_top = sf.next("{");
			std::string imagename_left = sf.next("{");
			std::string imagename_right = sf.next("{");

			// Generate images for the faces of the cube
			video::IImage *img_top = generate_image_from_scratch(
					imagename_top, device, sourcecache);
			video::IImage *img_left = generate_image_from_scratch(
					imagename_left, device, sourcecache);
			video::IImage *img_right = generate_image_from_scratch(
					imagename_right, device, sourcecache);
			assert(img_top && img_left && img_right);

			// Create textures from images
			video::ITexture *texture_top = driver->addTexture(
					(imagename_top + "__temp__").c_str(), img_top);
			video::ITexture *texture_left = driver->addTexture(
					(imagename_left + "__temp__").c_str(), img_left);
			video::ITexture *texture_right = driver->addTexture(
					(imagename_right + "__temp__").c_str(), img_right);
			assert(texture_top && texture_left && texture_right);

			// Drop images
			img_top->drop();
			img_left->drop();
			img_right->drop();
			
			/*
				Draw a cube mesh into a render target texture
			*/
			scene::IMesh* cube = createCubeMesh(v3f(1, 1, 1));
			setMeshColor(cube, video::SColor(255, 255, 255, 255));
			cube->getMeshBuffer(0)->getMaterial().setTexture(0, texture_top);
			cube->getMeshBuffer(1)->getMaterial().setTexture(0, texture_top);
			cube->getMeshBuffer(2)->getMaterial().setTexture(0, texture_right);
			cube->getMeshBuffer(3)->getMaterial().setTexture(0, texture_right);
			cube->getMeshBuffer(4)->getMaterial().setTexture(0, texture_left);
			cube->getMeshBuffer(5)->getMaterial().setTexture(0, texture_left);

			core::dimension2d<u32> dim(64,64);
			std::string rtt_texture_name = part_of_name + "_RTT";

			v3f camera_position(0, 1.0, -1.5);
			camera_position.rotateXZBy(45);
			v3f camera_lookat(0, 0, 0);
			core::CMatrix4<f32> camera_projection_matrix;
			// Set orthogonal projection
			camera_projection_matrix.buildProjectionMatrixOrthoLH(
					1.65, 1.65, 0, 100);

			video::SColorf ambient_light(0.2,0.2,0.2);
			v3f light_position(10, 100, -50);
			video::SColorf light_color(0.5,0.5,0.5);
			f32 light_radius = 1000;

			video::ITexture *rtt = generateTextureFromMesh(
					cube, device, dim, rtt_texture_name,
					camera_position,
					camera_lookat,
					camera_projection_matrix,
					ambient_light,
					light_position,
					light_color,
					light_radius);
			
			// Drop mesh
			cube->drop();

			// Free textures of images
			driver->removeTexture(texture_top);
			driver->removeTexture(texture_left);
			driver->removeTexture(texture_right);
			
			if(rtt == NULL)
			{
				errorstream<<"generate_image(): render to texture failed."
						" Creating fallback image"<<std::endl;
				baseimg = generate_image_from_scratch(
						imagename_top, device, sourcecache);
				return true;
			}

			// Create image of render target
			video::IImage *image = driver->createImage(rtt, v2s32(0,0), dim);
			assert(image);

			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);

			if(image)
			{
				image->copyTo(baseimg);
				image->drop();
			}
		}
		else
		{
			errorstream<<"generate_image(): Invalid "
					" modification: \""<<part_of_name<<"\""<<std::endl;
		}
	}

	return true;
}

void brighten(video::IImage *image)
{
	if(image == NULL)
		return;
	
	core::dimension2d<u32> dim = image->getDimension();

	for(u32 y=0; y<dim.Height; y++)
	for(u32 x=0; x<dim.Width; x++)
	{
		video::SColor c = image->getPixel(x,y);
		c.setRed(0.5 * 255 + 0.5 * (float)c.getRed());
		c.setGreen(0.5 * 255 + 0.5 * (float)c.getGreen());
		c.setBlue(0.5 * 255 + 0.5 * (float)c.getBlue());
		image->setPixel(x,y,c);
	}
}


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
#include <ICameraSceneNode.h>
#include "log.h"
#include "mapnode.h" // For texture atlas making
#include "mineral.h" // For texture atlas making

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
		std::string testpath = porting::getDataPath(filename.c_str());
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
	}
	
	// Add to cache (also an empty result is cached)
	g_texturename_to_path_cache.set(filename, fullpath);
	
	// Finally return it
	return fullpath;
}

/*
	TextureSource
*/

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

	// Build main texture atlas
	if(g_settings->getBool("enable_texture_atlas"))
		buildMainAtlas();
	else
		infostream<<"Not building texture atlas."<<std::endl;
}

TextureSource::~TextureSource()
{
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

		infostream<<"TextureSource::processQueue(): "
				<<"got texture request with "
				<<"name=\""<<request.key<<"\""
				<<std::endl;

		GetResult<std::string, u32, u8, u8>
				result;
		result.key = request.key;
		result.callers = request.callers;
		result.item = getTextureIdDirect(request.key);

		request.dest->push_back(result);
	}
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

// Draw a progress bar on the image
void make_progressbar(float value, video::IImage *image);

/*
	Generate image based on a string like "stone.png" or "[crack0".
	if baseimg is NULL, it is created. Otherwise stuff is made on it.
*/
bool generate_image(std::string part_of_name, video::IImage *& baseimg,
		IrrlichtDevice *device);

/*
	Generates an image from a full string like
	"stone.png^mineral_coal.png^[crack0".

	This is used by buildMainAtlas().
*/
video::IImage* generate_image_from_scratch(std::string name,
		IrrlichtDevice *device);

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
			infostream<<"getTextureIdDirect(): \""<<name
					<<"\" found in cache"<<std::endl;
			return n->getValue();
		}
	}

	infostream<<"getTextureIdDirect(): \""<<name
			<<"\" NOT found in cache. Creating it."<<std::endl;
	
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
			infostream<<"getTextureIdDirect(): NULL image in "
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
	if(generate_image(last_part_of_name, baseimg, m_device) == false)
	{
		infostream<<"getTextureIdDirect(): "
				"failed to generate \""<<last_part_of_name<<"\""
				<<std::endl;
	}

	// If no resulting image, print a warning
	if(baseimg == NULL)
	{
		infostream<<"getTextureIdDirect(): baseimg is NULL (attempted to"
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
		infostream<<"TextureSource::getTextureName(): id="<<id
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

void TextureSource::buildMainAtlas() 
{
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
		ContentFeatures *f = &content_features(j);
		for(core::map<std::string, bool>::Iterator
				i = f->used_texturenames.getIterator();
				i.atEnd() == false; i++)
		{
			std::string name = i.getNode()->getKey();
			sourcelist[name] = true;

			if(f->often_contains_mineral){
				for(int k=1; k<MINERAL_COUNT; k++){
					std::string mineraltexture = mineral_block_texture(k);
					std::string fulltexture = name + "^" + mineraltexture;
					sourcelist[fulltexture] = true;
				}
			}
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

		/*video::IImage *img = driver->createImageFromFile(
				getTexturePath(name.c_str()).c_str());
		if(img == NULL)
			continue;
		
		core::dimension2d<u32> dim = img->getDimension();
		// Make a copy with the right color format
		video::IImage *img2 =
				driver->createImage(video::ECF_A8R8G8B8, dim);
		img->copyTo(img2);
		img->drop();*/
		
		// Generate image by name
		video::IImage *img2 = generate_image_from_scratch(name, m_device);
		if(img2 == NULL)
		{
			infostream<<"TextureSource::buildMainAtlas(): Couldn't generate texture atlas: Couldn't generate image \""<<name<<"\""<<std::endl;
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
		
        infostream<<"TextureSource::buildMainAtlas(): Adding \""<<name
                <<"\" to texture atlas"<<std::endl;

		// Tile it a few times in the X direction
		u16 xwise_tiling = column_width / dim.Width;
		if(xwise_tiling > 16) // Limit to 16 (more gives no benefit)
			xwise_tiling = 16;
		for(u32 j=0; j<xwise_tiling; j++)
		{
			// Copy the copy to the atlas
			img2->copyToWithAlpha(atlas_img,
					pos_in_atlas + v2s32(j*dim.Width,0),
					core::rect<s32>(v2s32(0,0), dim),
					video::SColor(255,255,255,255),
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
		
		// Get next id
		u32 id = m_atlaspointer_cache.size();

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
		m_atlaspointer_cache.push_back(nap);
		m_name_to_id.insert(name, id);
			
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
		IrrlichtDevice *device)
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
		baseimg = generate_image_from_scratch(base_image_name, device);
	}
	
	/*
		Parse out the last part of the name of the image and act
		according to it
	*/

	std::string last_part_of_name = name.substr(last_separator_position+1);
	//infostream<<"last_part_of_name=\""<<last_part_of_name<<"\""<<std::endl;
	
	// Generate image according to part of name
	if(generate_image(last_part_of_name, baseimg, device) == false)
	{
		infostream<<"generate_image_from_scratch(): "
				"failed to generate \""<<last_part_of_name<<"\""
				<<std::endl;
		return NULL;
	}
	
	return baseimg;
}

bool generate_image(std::string part_of_name, video::IImage *& baseimg,
		IrrlichtDevice *device)
{
	video::IVideoDriver* driver = device->getVideoDriver();
	assert(driver);

	// Stuff starting with [ are special commands
	if(part_of_name[0] != '[')
	{
		// A normal texture; load it from a file
		std::string path = getTexturePath(part_of_name.c_str());
		/*infostream<<"generate_image(): Loading path \""<<path
				<<"\""<<std::endl;*/
		
		video::IImage *image = driver->createImageFromFile(path.c_str());

		if(image == NULL)
		{
			infostream<<"generate_image(): Could not load image \""
                    <<part_of_name<<"\" from path \""<<path<<"\""
					<<" while building texture"<<std::endl;

			//return false;

			infostream<<"generate_image(): Creating a dummy"
                    <<" image for \""<<part_of_name<<"\""<<std::endl;

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

		infostream<<"generate_image(): generating special "
				<<"modification \""<<part_of_name<<"\""
				<<std::endl;
		
		/*
			This is the simplest of all; it just adds stuff to the
			name so that a separate texture is created.

			It is used to make textures for stuff that doesn't want
			to implement getting the texture from a bigger texture
			atlas.
		*/
		if(part_of_name == "[forcesingle")
		{
		}
		/*
			[crackN
			Adds a cracking texture
		*/
		else if(part_of_name.substr(0,6) == "[crack")
		{
			if(baseimg == NULL)
			{
				infostream<<"generate_image(): baseimg==NULL "
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
			video::IImage *img_crack = driver->createImageFromFile(
					getTexturePath("crack.png").c_str());
		
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
				video::IImage *img = driver->createImageFromFile(
						getTexturePath(filename.c_str()).c_str());
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
			[progressbarN
			Adds a progress bar, 0.0 <= N <= 1.0
		*/
		else if(part_of_name.substr(0,12) == "[progressbar")
		{
			if(baseimg == NULL)
			{
				infostream<<"generate_image(): baseimg==NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			float value = stof(part_of_name.substr(12));
			make_progressbar(value, baseimg);
		}
		/*
			"[noalpha:filename.png"
			Use an image without it's alpha channel.
			Used for the leaves texture when in old leaves mode, so
			that the transparent parts don't look completely black 
			when simple alpha channel is used for rendering.
		*/
		else if(part_of_name.substr(0,8) == "[noalpha")
		{
			if(baseimg != NULL)
			{
				infostream<<"generate_image(): baseimg!=NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			std::string filename = part_of_name.substr(9);

			std::string path = getTexturePath(filename.c_str());

			infostream<<"generate_image(): Loading path \""<<path
					<<"\""<<std::endl;
			
			video::IImage *image = driver->createImageFromFile(path.c_str());
			
			if(image == NULL)
			{
				infostream<<"generate_image(): Loading path \""
						<<path<<"\" failed"<<std::endl;
			}
			else
			{
				core::dimension2d<u32> dim = image->getDimension();
				baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
				
				// Set alpha to full
				for(u32 y=0; y<dim.Height; y++)
				for(u32 x=0; x<dim.Width; x++)
				{
					video::SColor c = image->getPixel(x,y);
					c.setAlpha(255);
					image->setPixel(x,y,c);
				}
				// Blit
				image->copyTo(baseimg);

				image->drop();
			}
		}
		/*
			"[makealpha:R,G,B:filename.png"
			Use an image with converting one color to transparent.
		*/
		else if(part_of_name.substr(0,11) == "[makealpha:")
		{
			if(baseimg != NULL)
			{
				infostream<<"generate_image(): baseimg!=NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			Strfnd sf(part_of_name.substr(11));
			u32 r1 = stoi(sf.next(","));
			u32 g1 = stoi(sf.next(","));
			u32 b1 = stoi(sf.next(":"));
			std::string filename = sf.next("");

			std::string path = getTexturePath(filename.c_str());

			infostream<<"generate_image(): Loading path \""<<path
					<<"\""<<std::endl;
			
			video::IImage *image = driver->createImageFromFile(path.c_str());
			
			if(image == NULL)
			{
				infostream<<"generate_image(): Loading path \""
						<<path<<"\" failed"<<std::endl;
			}
			else
			{
				core::dimension2d<u32> dim = image->getDimension();
				baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
				
				// Blit
				image->copyTo(baseimg);

				image->drop();

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
		}
		/*
			"[makealpha2:R,G,B;R2,G2,B2:filename.png"
			Use an image with converting two colors to transparent.
		*/
		else if(part_of_name.substr(0,12) == "[makealpha2:")
		{
			if(baseimg != NULL)
			{
				infostream<<"generate_image(): baseimg!=NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			Strfnd sf(part_of_name.substr(12));
			u32 r1 = stoi(sf.next(","));
			u32 g1 = stoi(sf.next(","));
			u32 b1 = stoi(sf.next(";"));
			u32 r2 = stoi(sf.next(","));
			u32 g2 = stoi(sf.next(","));
			u32 b2 = stoi(sf.next(":"));
			std::string filename = sf.next("");

			std::string path = getTexturePath(filename.c_str());

			infostream<<"generate_image(): Loading path \""<<path
					<<"\""<<std::endl;
			
			video::IImage *image = driver->createImageFromFile(path.c_str());
			
			if(image == NULL)
			{
				infostream<<"generate_image(): Loading path \""
						<<path<<"\" failed"<<std::endl;
			}
			else
			{
				core::dimension2d<u32> dim = image->getDimension();
				baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);

				// Blit
				image->copyTo(baseimg);

				image->drop();
				
				for(u32 y=0; y<dim.Height; y++)
				for(u32 x=0; x<dim.Width; x++)
				{
					video::SColor c = baseimg->getPixel(x,y);
					u32 r = c.getRed();
					u32 g = c.getGreen();
					u32 b = c.getBlue();
					if(!(r == r1 && g == g1 && b == b1) &&
					   !(r == r2 && g == g2 && b == b2))
						continue;
					c.setAlpha(0);
					baseimg->setPixel(x,y,c);
				}
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
				infostream<<"generate_image(): baseimg!=NULL "
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

#if 1
			//TODO

			if(driver->queryFeature(video::EVDF_RENDER_TO_TARGET) == false)
			{
				infostream<<"generate_image(): EVDF_RENDER_TO_TARGET"
						" not supported. Creating fallback image"<<std::endl;
				baseimg = generate_image_from_scratch(
						imagename_top, device);
				return true;
			}
			
			u32 w0 = 64;
			u32 h0 = 64;
			//infostream<<"inventorycube w="<<w0<<" h="<<h0<<std::endl;
			core::dimension2d<u32> dim(w0,h0);
			
			// Generate images for the faces of the cube
			video::IImage *img_top = generate_image_from_scratch(
					imagename_top, device);
			video::IImage *img_left = generate_image_from_scratch(
					imagename_left, device);
			video::IImage *img_right = generate_image_from_scratch(
					imagename_right, device);
			assert(img_top && img_left && img_right);

			// TODO: Create textures from images
			video::ITexture *texture_top = driver->addTexture(
					(imagename_top + "__temp__").c_str(), img_top);
			assert(texture_top);
			
			// Drop images
			img_top->drop();
			img_left->drop();
			img_right->drop();
			
			// Create render target texture
			video::ITexture *rtt = NULL;
			std::string rtt_name = part_of_name + "_RTT";
			rtt = driver->addRenderTargetTexture(dim, rtt_name.c_str(),
					video::ECF_A8R8G8B8);
			assert(rtt);
			
			// Set render target
			driver->setRenderTarget(rtt, true, true,
					video::SColor(0,0,0,0));
			
			// Get a scene manager
			scene::ISceneManager *smgr_main = device->getSceneManager();
			assert(smgr_main);
			scene::ISceneManager *smgr = smgr_main->createNewSceneManager();
			assert(smgr);
			
			/*
				Create scene:
				- An unit cube is centered at 0,0,0
				- Camera looks at cube from Y+, Z- towards Y-, Z+
				NOTE: Cube has to be changed to something else because
				the textures cannot be set individually (or can they?)
			*/

			scene::ISceneNode* cube = smgr->addCubeSceneNode(1.0, NULL, -1,
					v3f(0,0,0), v3f(0, 45, 0));
			// Set texture of cube
			cube->setMaterialTexture(0, texture_top);
			//cube->setMaterialFlag(video::EMF_LIGHTING, false);
			cube->setMaterialFlag(video::EMF_ANTI_ALIASING, false);
			cube->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);

			scene::ICameraSceneNode* camera = smgr->addCameraSceneNode(0,
					v3f(0, 1.0, -1.5), v3f(0, 0, 0));
			// Set orthogonal projection
			core::CMatrix4<f32> pm;
			pm.buildProjectionMatrixOrthoLH(1.65, 1.65, 0, 100);
			camera->setProjectionMatrix(pm, true);

			/*scene::ILightSceneNode *light =*/ smgr->addLightSceneNode(0,
					v3f(-50, 100, 0), video::SColorf(0.5,0.5,0.5), 1000);

			smgr->setAmbientLight(video::SColorf(0.2,0.2,0.2));

			// Render scene
			driver->beginScene(true, true, video::SColor(0,0,0,0));
			smgr->drawAll();
			driver->endScene();
			
			// NOTE: The scene nodes should not be dropped, otherwise
			//       smgr->drop() segfaults
			/*cube->drop();
			camera->drop();
			light->drop();*/
			// Drop scene manager
			smgr->drop();
			
			// Unset render target
			driver->setRenderTarget(0, true, true, 0);

			//TODO: Free textures of images
			driver->removeTexture(texture_top);
			
			// Create image of render target
			video::IImage *image = driver->createImage(rtt, v2s32(0,0), dim);

			assert(image);
			
			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);

			if(image)
			{
				image->copyTo(baseimg);
				image->drop();
			}
#endif
		}
		else
		{
			infostream<<"generate_image(): Invalid "
					" modification: \""<<part_of_name<<"\""<<std::endl;
		}
	}

	return true;
}

void make_progressbar(float value, video::IImage *image)
{
	if(image == NULL)
		return;
	
	core::dimension2d<u32> size = image->getDimension();

	u32 barheight = size.Height/16;
	u32 barpad_x = size.Width/16;
	u32 barpad_y = size.Height/16;
	u32 barwidth = size.Width - barpad_x*2;
	v2u32 barpos(barpad_x, size.Height - barheight - barpad_y);

	u32 barvalue_i = (u32)(((float)barwidth * value) + 0.5);

	video::SColor active(255,255,0,0);
	video::SColor inactive(255,0,0,0);
	for(u32 x0=0; x0<barwidth; x0++)
	{
		video::SColor *c;
		if(x0 < barvalue_i)
			c = &active;
		else
			c = &inactive;
		u32 x = x0 + barpos.X;
		for(u32 y=barpos.Y; y<barpos.Y+barheight; y++)
		{
			image->setPixel(x,y, *c);
		}
	}
}


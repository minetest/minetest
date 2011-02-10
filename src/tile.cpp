/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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
	buildMainAtlas();
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

		dstream<<"INFO: TextureSource::processQueue(): "
				<<"got texture request with "
				<<"name="<<request.key
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
	//dstream<<"INFO: getTextureId(): name="<<name<<std::endl;

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
		dstream<<"INFO: getTextureId(): Queued: name="<<name<<std::endl;

		// We're gonna ask the result to be put into here
		ResultQueue<std::string, u32, u8, u8> result_queue;
		
		// Throw a request in
		m_get_texture_queue.add(name, 0, 0, &result_queue);
		
		dstream<<"INFO: Waiting for texture from main thread, name="
				<<name<<std::endl;
		
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
			dstream<<"WARNING: Waiting for texture timed out."<<std::endl;
			return 0;
		}
	}
	
	dstream<<"WARNING: getTextureId(): Failed"<<std::endl;

	return 0;
}

// Draw a progress bar on the image
void make_progressbar(float value, video::IImage *image);

/*
	Generate image based on a string like "stone.png" or "[crack0".
	if baseimg is NULL, it is created. Otherwise stuff is made on it.
*/
bool generate_image(std::string part_of_name, video::IImage *& baseimg,
		video::IVideoDriver* driver);

/*
	Generates an image from a full string like
	"stone.png^mineral_coal.png^[crack0".

	This is used by buildMainAtlas().
*/
video::IImage* generate_image_from_scratch(std::string name,
		video::IVideoDriver* driver);

/*
	This method generates all the textures
*/
u32 TextureSource::getTextureIdDirect(const std::string &name)
{
	dstream<<"INFO: getTextureIdDirect(): name="<<name<<std::endl;

	// Empty name means texture 0
	if(name == "")
	{
		dstream<<"INFO: getTextureIdDirect(): name is empty"<<std::endl;
		return 0;
	}
	
	/*
		Calling only allowed from main thread
	*/
	if(get_current_thread_id() != m_main_thread)
	{
		dstream<<"ERROR: TextureSource::getTextureIdDirect() "
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
			dstream<<"INFO: getTextureIdDirect(): name="<<name
					<<" found in cache"<<std::endl;
			return n->getValue();
		}
	}

	dstream<<"INFO: getTextureIdDirect(): name="<<name
			<<" NOT found in cache. Creating it."<<std::endl;
	
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
		dstream<<"INFO: getTextureIdDirect(): Calling itself recursively"
				" to get base image, name="<<base_image_name<<std::endl;
		base_image_id = getTextureIdDirect(base_image_name);
	}
	
	dstream<<"base_image_id="<<base_image_id<<std::endl;
	
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

		core::dimension2d<u32> dim = ap.intsize;

		baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);

		core::position2d<s32> pos_to(0,0);
		core::position2d<s32> pos_from = ap.intpos;
		
		image->copyTo(
				baseimg, // target
				v2s32(0,0), // position in target
				core::rect<s32>(pos_from, dim) // from
		);

		dstream<<"INFO: getTextureIdDirect(): Loaded \""
				<<base_image_name<<"\" from image cache"
				<<std::endl;
	}
	
	/*
		Parse out the last part of the name of the image and act
		according to it
	*/

	std::string last_part_of_name = name.substr(last_separator_position+1);
	dstream<<"last_part_of_name="<<last_part_of_name<<std::endl;
	
	// Generate image according to part of name
	if(generate_image(last_part_of_name, baseimg, driver) == false)
	{
		dstream<<"INFO: getTextureIdDirect(): "
				"failed to generate \""<<last_part_of_name<<"\""
				<<std::endl;
		return 0;
	}

	// If no resulting image, return NULL
	if(baseimg == NULL)
	{
		dstream<<"WARNING: getTextureIdDirect(): baseimg is NULL (attempted to"
				" create texture \""<<name<<"\""<<std::endl;
		return 0;
	}

	// Create texture from resulting image
	t = driver->addTexture(name.c_str(), baseimg);
	
	// If no texture
	if(t == NULL)
		return 0;

	/*
		Add texture to caches
	*/

	JMutexAutoLock lock(m_atlaspointer_cache_mutex);
	
	u32 id = m_atlaspointer_cache.size();
	AtlasPointer ap(id);
	ap.atlas = t;
	ap.pos = v2f(0,0);
	ap.size = v2f(1,1);
	ap.tiled = 0;
	SourceAtlasPointer nap(name, ap, baseimg, v2s32(0,0), baseimg->getDimension());
	m_atlaspointer_cache.push_back(nap);
	m_name_to_id.insert(name, id);

	dstream<<"INFO: getTextureIdDirect(): name="<<name
			<<": succesfully returning id="<<id<<std::endl;
	
	return id;
}

std::string TextureSource::getTextureName(u32 id)
{
	JMutexAutoLock lock(m_atlaspointer_cache_mutex);

	if(id >= m_atlaspointer_cache.size())
	{
		dstream<<"WARNING: TextureSource::getTextureName(): id="<<id
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
	dstream<<"TextureSource::buildMainAtlas()"<<std::endl;

	//return; // Disable (for testing)
	
	video::IVideoDriver* driver = m_device->getVideoDriver();
	assert(driver);

	JMutexAutoLock lock(m_atlaspointer_cache_mutex);

	// Create an image of the right size
	core::dimension2d<u32> atlas_dim(1024,1024);
	video::IImage *atlas_img =
			driver->createImage(video::ECF_A8R8G8B8, atlas_dim);

	/*
		A list of stuff to add. This should contain as much of the
		stuff shown in game as possible, to minimize texture changes.
	*/

	core::array<std::string> sourcelist;

	sourcelist.push_back("stone.png");
	sourcelist.push_back("mud.png");
	sourcelist.push_back("sand.png");
	sourcelist.push_back("grass.png");
	sourcelist.push_back("grass_footsteps.png");
	sourcelist.push_back("tree.png");
	sourcelist.push_back("tree_top.png");
	sourcelist.push_back("water.png");
	sourcelist.push_back("leaves.png");
	sourcelist.push_back("mud.png^grass_side.png");
	
	sourcelist.push_back("stone.png^mineral_coal.png");
	sourcelist.push_back("stone.png^mineral_iron.png");
	sourcelist.push_back("mud.png^mineral_coal.png");
	sourcelist.push_back("mud.png^mineral_iron.png");
	sourcelist.push_back("sand.png^mineral_coal.png");
	sourcelist.push_back("sand.png^mineral_iron.png");
	
	/*
		First pass: generate almost everything
	*/
	core::position2d<s32> pos_in_atlas(0,0);
	for(u32 i=0; i<sourcelist.size(); i++)
	{
		std::string name = sourcelist[i];

		/*video::IImage *img = driver->createImageFromFile(
				porting::getDataPath(name.c_str()).c_str());
		if(img == NULL)
			continue;
		
		core::dimension2d<u32> dim = img->getDimension();
		// Make a copy with the right color format
		video::IImage *img2 =
				driver->createImage(video::ECF_A8R8G8B8, dim);
		img->copyTo(img2);
		img->drop();*/
		
		// Generate image by name
		video::IImage *img2 = generate_image_from_scratch(name, driver);
		core::dimension2d<u32> dim = img2->getDimension();
		
		// Tile it a few times in the X direction
		u16 xwise_tiling = 16;
		for(u32 j=0; j<xwise_tiling; j++)
		{
			// Copy the copy to the atlas
			img2->copyToWithAlpha(atlas_img,
					pos_in_atlas + v2s32(j*dim.Width,0),
					core::rect<s32>(v2s32(0,0), dim),
					video::SColor(255,255,255,255),
					NULL);
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
		pos_in_atlas.Y += dim.Height;
	}

	/*
		Make texture
	*/
	video::ITexture *t = driver->addTexture("__main_atlas__", atlas_img);
	assert(t);

	/*
		Second pass: set texture pointer in generated AtlasPointers
	*/
	for(u32 i=0; i<sourcelist.size(); i++)
	{
		std::string name = sourcelist[i];
		u32 id = m_name_to_id[name];
		m_atlaspointer_cache[id].a.atlas = t;
	}

	/*
		Write image to file so that it can be inspected
	*/
	driver->writeImageToFile(atlas_img, 
			porting::getDataPath("main_atlas.png").c_str());
}

video::IImage* generate_image_from_scratch(std::string name,
		video::IVideoDriver* driver)
{
	dstream<<"INFO: generate_image_from_scratch(): "
			"name="<<name<<std::endl;
	
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

	/*dstream<<"INFO: generate_image_from_scratch(): "
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
		dstream<<"INFO: generate_image_from_scratch(): Calling itself recursively"
				" to get base image, name="<<base_image_name<<std::endl;
		baseimg = generate_image_from_scratch(base_image_name, driver);
	}
	
	/*
		Parse out the last part of the name of the image and act
		according to it
	*/

	std::string last_part_of_name = name.substr(last_separator_position+1);
	dstream<<"last_part_of_name="<<last_part_of_name<<std::endl;
	
	// Generate image according to part of name
	if(generate_image(last_part_of_name, baseimg, driver) == false)
	{
		dstream<<"INFO: generate_image_from_scratch(): "
				"failed to generate \""<<last_part_of_name<<"\""
				<<std::endl;
		return NULL;
	}
	
	return baseimg;
}

bool generate_image(std::string part_of_name, video::IImage *& baseimg,
		video::IVideoDriver* driver)
{
	// Stuff starting with [ are special commands
	if(part_of_name[0] != '[')
	{
		// A normal texture; load it from a file
		std::string path = porting::getDataPath(part_of_name.c_str());
		dstream<<"INFO: getTextureIdDirect(): Loading path \""<<path
				<<"\""<<std::endl;
		
		video::IImage *image = driver->createImageFromFile(path.c_str());

		if(image == NULL)
		{
			dstream<<"WARNING: Could not load image \""<<part_of_name
					<<"\" from path \""<<path<<"\""
					<<" while building texture"<<std::endl;
			return false;
		}

		// If base image is NULL, load as base.
		if(baseimg == NULL)
		{
			dstream<<"INFO: Setting "<<part_of_name<<" as base"<<std::endl;
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
			dstream<<"INFO: Blitting "<<part_of_name<<" on base"<<std::endl;
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

		dstream<<"INFO: getTextureIdDirect(): generating special "
				<<"modification \""<<part_of_name<<"\""
				<<std::endl;

		if(part_of_name.substr(0,6) == "[crack")
		{
			if(baseimg == NULL)
			{
				dstream<<"WARNING: getTextureIdDirect(): baseimg==NULL "
						<<"for part_of_name="<<part_of_name
						<<", cancelling."<<std::endl;
				return false;
			}

			u16 progression = stoi(part_of_name.substr(6));
			// Size of the base image
			core::dimension2d<u32> dim_base = baseimg->getDimension();
			// Crack will be drawn at this size
			u32 cracksize = 16;
			// Size of the crack image
			core::dimension2d<u32> dim_crack(cracksize,cracksize);
			// Position to copy the crack from in the crack image
			core::position2d<s32> pos_other(0, 16 * progression);

			video::IImage *crackimage = driver->createImageFromFile(
					porting::getDataPath("crack.png").c_str());
		
			if(crackimage)
			{
				/*crackimage->copyToWithAlpha(baseimg, v2s32(0,0),
						core::rect<s32>(pos_other, dim_base),
						video::SColor(255,255,255,255),
						NULL);*/

				for(u32 y0=0; y0<dim_base.Height/dim_crack.Height; y0++)
				for(u32 x0=0; x0<dim_base.Width/dim_crack.Width; x0++)
				{
					// Position to copy the crack to in the base image
					core::position2d<s32> pos_base(x0*cracksize, y0*cracksize);
					crackimage->copyToWithAlpha(baseimg, pos_base,
							core::rect<s32>(pos_other, dim_crack),
							video::SColor(255,255,255,255),
							NULL);
				}

				crackimage->drop();
			}
		}
		else if(part_of_name.substr(0,8) == "[combine")
		{
			// "[combine:16x128:0,0=stone.png:0,16=grass.png"
			Strfnd sf(part_of_name);
			sf.next(":");
			u32 w0 = stoi(sf.next("x"));
			u32 h0 = stoi(sf.next(":"));
			dstream<<"INFO: combined w="<<w0<<" h="<<h0<<std::endl;
			core::dimension2d<u32> dim(w0,h0);
			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
			while(sf.atend() == false)
			{
				u32 x = stoi(sf.next(","));
				u32 y = stoi(sf.next("="));
				std::string filename = sf.next(":");
				dstream<<"INFO: Adding \""<<filename
						<<"\" to combined ("<<x<<","<<y<<")"
						<<std::endl;
				video::IImage *img = driver->createImageFromFile(
						porting::getDataPath(filename.c_str()).c_str());
				if(img)
				{
					core::dimension2d<u32> dim = img->getDimension();
					dstream<<"INFO: Size "<<dim.Width
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
					dstream<<"WARNING: img==NULL"<<std::endl;
				}
			}
		}
		else if(part_of_name.substr(0,12) == "[progressbar")
		{
			if(baseimg == NULL)
			{
				dstream<<"WARNING: getTextureIdDirect(): baseimg==NULL "
						<<"for part_of_name="<<part_of_name
						<<", cancelling."<<std::endl;
				return false;
			}

			float value = stof(part_of_name.substr(12));
			make_progressbar(value, baseimg);
		}
		// "[noalpha:filename.png"
		// Use an image without it's alpha channel
		else if(part_of_name.substr(0,8) == "[noalpha")
		{
			if(baseimg != NULL)
			{
				dstream<<"WARNING: getTextureIdDirect(): baseimg!=NULL "
						<<"for part_of_name="<<part_of_name
						<<", cancelling."<<std::endl;
				return false;
			}

			std::string filename = part_of_name.substr(9);

			std::string path = porting::getDataPath(filename.c_str());

			dstream<<"INFO: getTextureIdDirect(): Loading path \""<<path
					<<"\""<<std::endl;
			
			video::IImage *image = driver->createImageFromFile(path.c_str());
			
			if(image == NULL)
			{
				dstream<<"WARNING: getTextureIdDirect(): Loading path \""
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
		else
		{
			dstream<<"WARNING: getTextureIdDirect(): Invalid "
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

	u32 barheight = 1;
	u32 barpad_x = 1;
	u32 barpad_y = 1;
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


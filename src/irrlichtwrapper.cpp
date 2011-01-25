#include "irrlichtwrapper.h"
#include "constants.h"
#include "string.h"
#include "strfnd.h"

IrrlichtWrapper::IrrlichtWrapper(IrrlichtDevice *device)
{
	m_main_thread = get_current_thread_id();
	m_device_mutex.Init();
	m_device = device;
}

void IrrlichtWrapper::Run()
{
	/*
		Fetch textures
	*/
	if(m_get_texture_queue.size() > 0)
	{
		GetRequest<std::string, video::ITexture*, u8, u8>
				request = m_get_texture_queue.pop();

		dstream<<"got texture request with key="
				<<request.key<<std::endl;

		GetResult<std::string, video::ITexture*, u8, u8>
				result;
		result.key = request.key;
		result.callers = request.callers;
		result.item = getTextureDirect(request.key);

		request.dest->push_back(result);
	}
}

video::ITexture* IrrlichtWrapper::getTexture(const std::string &spec)
{
	if(spec == "")
		return NULL;
	
	video::ITexture *t = m_texturecache.get(spec);
	if(t != NULL)
		return t;
	
	if(get_current_thread_id() == m_main_thread)
	{
		dstream<<"Getting texture directly: spec="
				<<spec<<std::endl;
				
		t = getTextureDirect(spec);
	}
	else
	{
		// We're gonna ask the result to be put into here
		ResultQueue<std::string, video::ITexture*, u8, u8> result_queue;
		
		// Throw a request in
		m_get_texture_queue.add(spec, 0, 0, &result_queue);
		
		dstream<<"Waiting for texture from main thread: "
				<<spec<<std::endl;
		
		try
		{
			// Wait result for a second
			GetResult<std::string, video::ITexture*, u8, u8>
					result = result_queue.pop_front(1000);
		
			// Check that at least something worked OK
			assert(result.key == spec);

			t = result.item;
		}
		catch(ItemNotFoundException &e)
		{
			dstream<<"Waiting for texture timed out."<<std::endl;
			t = NULL;
		}
	}

	// Add to cache and return
	m_texturecache.set(spec, t);
	return t;
}

/*
	Non-thread-safe functions
*/

/*
	Texture modifier functions
*/

// blitted_name = eg. "mineral_coal.png"
video::ITexture * make_blitname(const std::string &blitted_name,
		video::ITexture *original,
		const char *newname, video::IVideoDriver* driver)
{
	if(original == NULL)
		return NULL;
	
	// Size of the base image
	core::dimension2d<u32> dim(16, 16);
	// Position to copy the blitted to in the base image
	core::position2d<s32> pos_base(0, 0);
	// Position to copy the blitted from in the blitted image
	core::position2d<s32> pos_other(0, 0);

	video::IImage *baseimage = driver->createImage(original, pos_base, dim);
	assert(baseimage);

	video::IImage *blittedimage = driver->createImageFromFile(porting::getDataPath(blitted_name.c_str()).c_str());
	assert(blittedimage);
	
	// Then copy the right part of blittedimage to baseimage
	
	blittedimage->copyToWithAlpha(baseimage, v2s32(0,0),
			core::rect<s32>(pos_other, dim),
			video::SColor(255,255,255,255),
			NULL);
	
	blittedimage->drop();

	// Create texture from resulting image

	video::ITexture *newtexture = driver->addTexture(newname, baseimage);

	baseimage->drop();

	return newtexture;
}

video::ITexture * make_crack(u16 progression, video::ITexture *original,
		const char *newname, video::IVideoDriver* driver)
{
	if(original == NULL)
		return NULL;
	
	// Size of the base image
	core::dimension2d<u32> dim(16, 16);
	// Size of the crack image
	//core::dimension2d<u32> dim_crack(16, 16 * CRACK_ANIMATION_LENGTH);
	// Position to copy the crack to in the base image
	core::position2d<s32> pos_base(0, 0);
	// Position to copy the crack from in the crack image
	core::position2d<s32> pos_other(0, 16 * progression);

	video::IImage *baseimage = driver->createImage(original, pos_base, dim);
	assert(baseimage);

	video::IImage *crackimage = driver->createImageFromFile(porting::getDataPath("crack.png").c_str());
	assert(crackimage);
	
	// Then copy the right part of crackimage to baseimage
	
	crackimage->copyToWithAlpha(baseimage, v2s32(0,0),
			core::rect<s32>(pos_other, dim),
			video::SColor(255,255,255,255),
			NULL);
	
	crackimage->drop();

	// Create texture from resulting image

	video::ITexture *newtexture = driver->addTexture(newname, baseimage);

	baseimage->drop();

	return newtexture;
}

#if 0
video::ITexture * make_sidegrass(video::ITexture *original,
		const char *newname, video::IVideoDriver* driver)
{
	if(original == NULL)
		return NULL;
	
	// Size of the base image
	core::dimension2d<u32> dim(16, 16);
	// Position to copy the grass to in the base image
	core::position2d<s32> pos_base(0, 0);
	// Position to copy the grass from in the grass image
	core::position2d<s32> pos_other(0, 0);

	video::IImage *baseimage = driver->createImage(original, pos_base, dim);
	assert(baseimage);

	video::IImage *grassimage = driver->createImageFromFile(porting::getDataPath("grass_side.png").c_str());
	assert(grassimage);
	
	// Then copy the right part of grassimage to baseimage
	
	grassimage->copyToWithAlpha(baseimage, v2s32(0,0),
			core::rect<s32>(pos_other, dim),
			video::SColor(255,255,255,255),
			NULL);
	
	grassimage->drop();

	// Create texture from resulting image

	video::ITexture *newtexture = driver->addTexture(newname, baseimage);

	baseimage->drop();

	return newtexture;
}
#endif

video::ITexture * make_progressbar(float value, video::ITexture *original,
		const char *newname, video::IVideoDriver* driver)
{
	if(original == NULL)
		return NULL;
	
	core::position2d<s32> pos_base(0, 0);
	core::dimension2d<u32> dim = original->getOriginalSize();

	video::IImage *baseimage = driver->createImage(original, pos_base, dim);
	assert(baseimage);
	
	core::dimension2d<u32> size = baseimage->getDimension();

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
			baseimage->setPixel(x,y, *c);
		}
	}
	
	video::ITexture *newtexture = driver->addTexture(newname, baseimage);

	baseimage->drop();

	return newtexture;
}

/*
	Texture fetcher/maker function, called always from the main thread
*/

video::ITexture* IrrlichtWrapper::getTextureDirect(const std::string &spec)
{
	if(spec == "")
		return NULL;

	video::IVideoDriver* driver = m_device->getVideoDriver();
	
	/*
		Input (spec) is something like this:
		"/usr/share/minetest/stone.png[[mod:mineral0[[mod:crack3"
	*/
	
	video::ITexture* t = NULL;
	std::string modmagic = "[[mod:";
	Strfnd f(spec);
	std::string path = f.next(modmagic);
	t = driver->getTexture(path.c_str());
	std::string texture_name = path;
	while(f.atend() == false)
	{
		std::string mod = f.next(modmagic);
		texture_name += modmagic + mod;
		dstream<<"Making texture \""<<texture_name<<"\""<<std::endl;
		/*if(mod == "sidegrass")
		{
			t = make_sidegrass(t, texture_name.c_str(), driver);
		}
		else*/
		if(mod.substr(0, 9) == "blitname:")
		{
			//t = make_sidegrass(t, texture_name.c_str(), driver);
			t = make_blitname(mod.substr(9), t, texture_name.c_str(), driver);
		}
		else if(mod.substr(0,5) == "crack")
		{
			u16 prog = stoi(mod.substr(5));
			t = make_crack(prog, t, texture_name.c_str(), driver);
		}
		else if(mod.substr(0,11) == "progressbar")
		{
			float value = stof(mod.substr(11));
			t = make_progressbar(value, t, texture_name.c_str(), driver);
		}
		else
		{
			dstream<<"Invalid texture mod: \""<<mod<<"\""<<std::endl;
		}
	}
	return t;

#if 0
	video::ITexture* t = NULL;
	const char *modmagic = "[[mod:";
	const s32 modmagic_len = 6;
	enum{
		READMODE_PATH,
		READMODE_MOD
	} readmode = READMODE_PATH;
	s32 specsize = spec.size()+1;
	char *strcache = (char*)malloc(specsize);
	assert(strcache);
	char *path = NULL;
	s32 length = 0;
	// Next index of modmagic to be found
	s32 modmagic_i = 0;
	u32 i=0;
	for(;;)
	{
		strcache[length++] = spec[i];
		
		bool got_modmagic = false;

		/*
			Check modmagic
		*/
		if(spec[i] == modmagic[modmagic_i])
		{
			modmagic_i++;
			if(modmagic_i == modmagic_len)
			{
				got_modmagic = true;
				modmagic_i = 0;
				length -= modmagic_len;
			}
		}
		else
			modmagic_i = 0;
		
		// Set i to be the length of read string
		i++;

		if(got_modmagic || i >= spec.size())
		{
			strcache[length] = '\0';
			// Now our string is in strcache, ending in \0
			
			if(readmode == READMODE_PATH)
			{
				// Get initial texture (strcache is path)
				assert(t == NULL);
				t = driver->getTexture(strcache);
				readmode = READMODE_MOD;
				path = strcache;
				strcache = (char*)malloc(specsize);
				assert(strcache);
			}
			else
			{
				dstream<<"Parsing mod \""<<strcache<<"\""<<std::endl;
				// The name of the result of adding this mod.
				// This doesn't have to be fast so std::string is used.
				std::string name(path);
				name += "[[mod:";
				name += strcache;
				dstream<<"Name of modded texture is \""<<name<<"\""
						<<std::endl;
				// Sidegrass
				if(strcmp(strcache, "sidegrass") == 0)
				{
					t = make_sidegrass(t, name.c_str(), driver);
				}
				else
				{
					dstream<<"Invalid texture mod"<<std::endl;
				}
			}

			length = 0;
		}

		if(i >= spec.size())
			break;
	}

	/*if(spec.mod == NULL)
	{
		dstream<<"IrrlichtWrapper::getTextureDirect: Loading texture "
				<<spec.path<<std::endl;
		return driver->getTexture(spec.path.c_str());
	}

	dstream<<"IrrlichtWrapper::getTextureDirect: Loading and modifying "
			"texture "<<spec.path<<" to make "<<spec.name<<std::endl;

	video::ITexture *base = driver->getTexture(spec.path.c_str());
	video::ITexture *result = spec.mod->make(base, spec.name.c_str(), driver);

	delete spec.mod;*/
	
	if(strcache)
		free(strcache);
	if(path)
		free(path);
	
	return t;
#endif
}



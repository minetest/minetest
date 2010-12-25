#include "irrlichtwrapper.h"
#include "constants.h"

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
		GetRequest<TextureSpec, video::ITexture*, u8, u8>
				request = m_get_texture_queue.pop();

		dstream<<"got texture request with key.name="
				<<request.key.name<<std::endl;

		GetResult<TextureSpec, video::ITexture*, u8, u8>
				result;
		result.key = request.key;
		result.callers = request.callers;
		result.item = getTextureDirect(request.key);

		request.dest->push_back(result);
	}
}

video::ITexture* IrrlichtWrapper::getTexture(TextureSpec spec)
{
	video::ITexture *t = m_texturecache.get(spec.name);
	if(t != NULL)
		return t;
	
	if(get_current_thread_id() == m_main_thread)
	{
		dstream<<"Getting texture directly: name="
				<<spec.name<<std::endl;
				
		t = getTextureDirect(spec);
	}
	else
	{
		// We're gonna ask the result to be put into here
		ResultQueue<TextureSpec, video::ITexture*, u8, u8> result_queue;
		
		// Throw a request in
		m_get_texture_queue.add(spec, 0, 0, &result_queue);
		
		dstream<<"Waiting for texture from main thread: "
				<<spec.name<<std::endl;
		
		try
		{
			// Wait result for a second
			GetResult<TextureSpec, video::ITexture*, u8, u8>
					result = result_queue.pop_front(1000);
		
			// Check that at least something worked OK
			assert(result.key.name == spec.name);

			t = result.item;
		}
		catch(ItemNotFoundException &e)
		{
			dstream<<"Waiting for texture timed out."<<std::endl;
			t = NULL;
		}
	}

	// Add to cache and return
	m_texturecache.set(spec.name, t);
	return t;
}

video::ITexture* IrrlichtWrapper::getTexture(const std::string &path)
{
	return getTexture(TextureSpec(path, path, NULL));
}

/*
	Non-thread-safe functions
*/

video::ITexture* IrrlichtWrapper::getTextureDirect(TextureSpec spec)
{
	video::IVideoDriver* driver = m_device->getVideoDriver();
	
	if(spec.mod == NULL)
	{
		dstream<<"IrrlichtWrapper::getTextureDirect: Loading texture "
				<<spec.path<<std::endl;
		return driver->getTexture(spec.path.c_str());
	}

	dstream<<"IrrlichtWrapper::getTextureDirect: Loading and modifying "
			"texture "<<spec.path<<" to make "<<spec.name<<std::endl;

	video::ITexture *base = driver->getTexture(spec.path.c_str());
	video::ITexture *result = spec.mod->make(base, spec.name.c_str(), driver);

	delete spec.mod;
	
	return result;
}

video::ITexture * CrackTextureMod::make(video::ITexture *original,
		const char *newname, video::IVideoDriver* driver)
{
	core::dimension2d<u32> dim(16, 16);
	core::position2d<s32> pos_base(0, 0);
	core::position2d<s32> pos_other(0, 16 * progression);

	video::IImage *baseimage = driver->createImage(original, pos_base, dim);
	assert(baseimage);
	
	video::ITexture *other = driver->getTexture("../data/crack.png");
	
	dstream<<__FUNCTION_NAME<<": crack texture size is "
			<<other->getSize().Width<<"x"
			<<other->getSize().Height<<std::endl;

	// We have to get the whole texture because getting a smaller area
	// messes the whole thing. It is probably a bug in Irrlicht.
	// NOTE: This doesn't work probably because some systems scale
	//       the image to fit a texture or something...
	video::IImage *otherimage = driver->createImage(
			other, core::position2d<s32>(0,0), other->getSize());
	// This should work on more systems
	// - no, it doesn't, output is more random.
	/*video::IImage *otherimage = driver->createImage(
			other, core::position2d<s32>(0,0),
			v2u32(16, CRACK_ANIMATION_LENGTH * 16));*/

	assert(otherimage);
	
	/*core::rect<s32> clip_rect(v2s32(0,0), dim);
	otherimage->copyToWithAlpha(baseimage, v2s32(0,0),
			core::rect<s32>(pos_other, dim),
			video::SColor(255,255,255,255),
			&clip_rect);*/
	
	otherimage->copyToWithAlpha(baseimage, v2s32(0,0),
			core::rect<s32>(pos_other, dim),
			video::SColor(255,255,255,255),
			NULL);
	
	otherimage->drop();

	video::ITexture *newtexture = driver->addTexture(newname, baseimage);

	baseimage->drop();

	return newtexture;
}

video::ITexture * ProgressBarTextureMod::make(video::ITexture *original,
		const char *newname, video::IVideoDriver* driver)
{
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


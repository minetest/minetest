#include "irrlichtwrapper.h"

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
		
		dstream<<"Waiting for texture "<<spec.name<<std::endl;

		// Wait result
		GetResult<TextureSpec, video::ITexture*, u8, u8>
				result = result_queue.pop_front(1000);
		
		// Check that at least something worked OK
		assert(result.key.name == spec.name);

		t = result.item;
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
	// We have to get the whole texture because getting a smaller area
	// messes the whole thing. It is probably a bug in Irrlicht.
	video::IImage *otherimage = driver->createImage(
			other, core::position2d<s32>(0,0), other->getSize());

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

#if 0
video::ITexture * createAlphaBlitTexture(const char *name, video::ITexture *base,
		video::ITexture *other, v2u32 size, v2s32 pos_base, v2s32 pos_other)
{
	if(g_device == NULL)
		return NULL;
	video::IVideoDriver* driver = g_device->getVideoDriver();

	core::dimension2d<u32> dim(size.X, size.Y);

	video::IImage *baseimage = driver->createImage(
			base,
			core::position2d<s32>(pos_base.X, pos_base.Y),
			dim);
	assert(baseimage);

	video::IImage *otherimage = driver->createImage(
			other,
			core::position2d<s32>(pos_other.X, pos_other.Y),
			dim);
	assert(sourceimage);
	
	otherimage->copyToWithAlpha(baseimage, v2s32(0,0),
			core::rect<s32>(v2s32(0,0), dim),
			video::SColor(255,255,255,255),
			core::rect<s32>(v2s32(0,0), dim));
	otherimage->drop();

	video::ITexture *newtexture = driver->addTexture(name, baseimage);

	baseimage->drop();

	return newtexture;
}
#endif


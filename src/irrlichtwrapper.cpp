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
		GetRequest<TextureSpec, video::ITexture*, u8, u8>
				request = m_get_texture_queue.pop();

		dstream<<"got texture request with"
				<<" key.tids[0]="<<request.key.tids[0]
				<<" [1]="<<request.key.tids[1]
				<<std::endl;

		GetResult<TextureSpec, video::ITexture*, u8, u8>
				result;
		result.key = request.key;
		result.callers = request.callers;
		result.item = getTextureDirect(request.key);

		request.dest->push_back(result);
	}
}

textureid_t IrrlichtWrapper::getTextureId(const std::string &name)
{
	u32 id = m_namecache.getId(name);
	return id;
}

std::string IrrlichtWrapper::getTextureName(textureid_t id)
{
	std::string name("");
	m_namecache.getValue(id, name);
	// In case it was found, return the name; otherwise return an empty name.
	return name;
}

video::ITexture* IrrlichtWrapper::getTexture(const std::string &name)
{
	TextureSpec spec(getTextureId(name));
	return getTexture(spec);
}

video::ITexture* IrrlichtWrapper::getTexture(const TextureSpec &spec)
{
	if(spec.empty())
		return NULL;
	
	video::ITexture *t = m_texturecache.get(spec);
	if(t != NULL)
		return t;
	
	if(get_current_thread_id() == m_main_thread)
	{
		dstream<<"Getting texture directly: spec.tids[0]="
				<<spec.tids[0]<<std::endl;
				
		t = getTextureDirect(spec);
	}
	else
	{
		// We're gonna ask the result to be put into here
		ResultQueue<TextureSpec, video::ITexture*, u8, u8> result_queue;
		
		// Throw a request in
		m_get_texture_queue.add(spec, 0, 0, &result_queue);
		
		dstream<<"Waiting for texture from main thread: spec.tids[0]="
				<<spec.tids[0]<<std::endl;
		
		try
		{
			// Wait result for a second
			GetResult<TextureSpec, video::ITexture*, u8, u8>
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

// Draw a progress bar on the image
void make_progressbar(float value, video::IImage *image);

/*
	Texture fetcher/maker function, called always from the main thread
*/

video::ITexture* IrrlichtWrapper::getTextureDirect(const TextureSpec &spec)
{
	// This would result in NULL image
	if(spec.empty())
		return NULL;
	
	// Don't generate existing stuff
	video::ITexture *t = m_texturecache.get(spec);
	if(t != NULL)
	{
		dstream<<"WARNING: Existing stuff requested from "
				"getTextureDirect()"<<std::endl;
		return t;
	}
	
	video::IVideoDriver* driver = m_device->getVideoDriver();

	/*
		An image will be built from files and then converted into a texture.
	*/
	video::IImage *baseimg = NULL;

	/*
		Irrlicht requires a name for every texture, with which it
		will be stored internally in irrlicht.
	*/
	std::string texture_name;

	for(u32 i=0; i<TEXTURE_SPEC_TEXTURE_COUNT; i++)
	{
		textureid_t tid = spec.tids[i];
		if(tid == 0)
			continue;

		std::string name = getTextureName(tid);
		
		// Add something to the name so that it is a unique identifier.
		texture_name += "[";
		texture_name += name;
		texture_name += "]";

		if(name[0] != '[')
		{
			// A normal texture; load it from a file
			std::string path = porting::getDataPath(name.c_str());
			dstream<<"getTextureDirect(): Loading path \""<<path
					<<"\""<<std::endl;
			video::IImage *image = driver->createImageFromFile(path.c_str());

			if(image == NULL)
			{
				dstream<<"WARNING: Could not load image \""<<name
						<<"\" from path \""<<path<<"\""
						<<" while building texture"<<std::endl;
				continue;
			}

			// If base image is NULL, load as base.
			if(baseimg == NULL)
			{
				dstream<<"Setting "<<name<<" as base"<<std::endl;
				/*
					Copy it this way to get an alpha channel.
					Otherwise images with alpha cannot be blitted on 
					images that don't have alpha in the original file.
				*/
				// This is a deprecated method
				//baseimg = driver->createImage(video::ECF_A8R8G8B8, image);
				core::dimension2d<u32> dim = image->getDimension();
				baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
				image->copyTo(baseimg);
				image->drop();
				//baseimg = image;
			}
			// Else blit on base.
			else
			{
				dstream<<"Blitting "<<name<<" on base"<<std::endl;
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
			dstream<<"getTextureDirect(): generating \""<<name<<"\""
					<<std::endl;
			if(name.substr(0,6) == "[crack")
			{
				u16 progression = stoi(name.substr(6));
				// Size of the base image
				core::dimension2d<u32> dim(16, 16);
				// Size of the crack image
				//core::dimension2d<u32> dim_crack(16, 16 * CRACK_ANIMATION_LENGTH);
				// Position to copy the crack to in the base image
				core::position2d<s32> pos_base(0, 0);
				// Position to copy the crack from in the crack image
				core::position2d<s32> pos_other(0, 16 * progression);

				video::IImage *crackimage = driver->createImageFromFile(
						porting::getDataPath("crack.png").c_str());
				crackimage->copyToWithAlpha(baseimg, v2s32(0,0),
						core::rect<s32>(pos_other, dim),
						video::SColor(255,255,255,255),
						NULL);
				crackimage->drop();
			}
			else if(name.substr(0,12) == "[progressbar")
			{
				float value = stof(name.substr(12));
				make_progressbar(value, baseimg);
			}
			else
			{
				dstream<<"WARNING: getTextureDirect(): Invalid "
						" texture: \""<<name<<"\""<<std::endl;
			}
		}
	}

	// If no resulting image, return NULL
	if(baseimg == NULL)
	{
		dstream<<"getTextureDirect(): baseimg is NULL (attempted to"
				" create texture \""<<texture_name<<"\""<<std::endl;
		return NULL;
	}
	
	/*// DEBUG: Paint some pixels
	video::SColor c(255,255,0,0);
	baseimg->setPixel(1,1, c);
	baseimg->setPixel(1,14, c);
	baseimg->setPixel(14,1, c);
	baseimg->setPixel(14,14, c);*/

	// Create texture from resulting image
	t = driver->addTexture(texture_name.c_str(), baseimg);
	baseimg->drop();

	dstream<<"getTextureDirect(): created texture \""<<texture_name
			<<"\""<<std::endl;

	return t;

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



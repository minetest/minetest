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

#ifndef IRRLICHTWRAPPER_HEADER
#define IRRLICHTWRAPPER_HEADER

#include "threads.h"
#include "common_irrlicht.h"
#include "debug.h"
#include "utility.h"
#include "texture.h"
#include "iirrlichtwrapper.h"

#include <jmutex.h>
#include <jmutexautolock.h>
#include <string>

/*
	NOTE: This is deprecated and should be removed completely
*/

/*
	A thread-safe texture pointer cache.
	
	This is used so that irrlicht doesn't get called from many
	threads, because texture pointers have to be handled in
	background threads.
*/

#if 0
/*
	A thread-safe texture pointer cache
*/
class TextureCache
{
public:
	TextureCache()
	{
		m_mutex.Init();
		assert(m_mutex.IsInitialized());
	}
	
	void set(const TextureSpec &spec, video::ITexture *texture)
	{
		if(texture == NULL)
			return;
		
		JMutexAutoLock lock(m_mutex);

		m_textures[spec] = texture;
	}
	
	video::ITexture* get(const TextureSpec &spec)
	{
		JMutexAutoLock lock(m_mutex);

		core::map<TextureSpec, video::ITexture*>::Node *n;
		n = m_textures.find(spec);

		if(n != NULL)
			return n->getValue();

		return NULL;
	}

private:
	core::map<TextureSpec, video::ITexture*> m_textures;
	JMutex m_mutex;
};
#endif

/*
	A thread-safe wrapper for irrlicht, to be accessed from
	background worker threads.

	Queues tasks to be done in the main thread.

	Also caches texture specification strings to ids and textures.

	TODO: Remove this and move all texture functionality to TextureSource
*/

class IrrlichtWrapper : public IIrrlichtWrapper
{
public:
	/*
		These are called from the main thread
	*/

	IrrlichtWrapper(IrrlichtDevice *device);

	~IrrlichtWrapper();
	
	// Run queued tasks
	void Run();

	// Shutdown wrapper; this disables queued texture fetching
	void Shutdown(bool shutdown);

	IrrlichtDevice* getDevice();
	
	/*
		These are called from other threads
	*/

	// Not exactly thread-safe but this needs to be fast.
	// getTimer()->getRealTime() only reads one variable anyway.
	u32 getTime()
	{
		return m_device->getTimer()->getRealTime();
	}

#if 0
	/*
		Format of a texture name:
			"stone.png" (filename in image data directory)
			"[crack1" (a name starting with "[" is a special feature)
			"[progress1.0" (a name starting with "[" is a special feature)
	*/
	/*
		Loads texture defined by "name" and assigns a texture id to it.
		If texture has to be generated, generates it.
		If the texture has already been loaded, returns existing id.
	*/
	textureid_t getTextureId(const std::string &name);
	// The reverse of the above
	std::string getTextureName(textureid_t id);
	// Gets a texture based on a filename
	video::ITexture* getTexture(const std::string &filename);
	// Gets a texture based on a TextureSpec (a textureid_t is fine too)
	video::ITexture* getTexture(const TextureSpec &spec);
#endif
	
private:
	/*
		Non-thread-safe variants of stuff, for internal use
	*/

	// Constructs a texture according to spec
	//video::ITexture* getTextureDirect(const TextureSpec &spec);
	
	/*
		Members
	*/

	bool m_running;
	
	// The id of the thread that can (and has to) use irrlicht directly
	threadid_t m_main_thread;
	
	// The irrlicht device
	JMutex m_device_mutex;
	IrrlichtDevice *m_device;
	
#if 0
	// Queued texture fetches (to be processed by the main thread)
	RequestQueue<TextureSpec, video::ITexture*, u8, u8> m_get_texture_queue;

	// Cache of textures by spec
	TextureCache m_texturecache;

	// Cached or generated source images by texture name
	core::map<std::string, video::IImage*> m_imagecache;

	// A mapping from texture id to string spec
	MutexedIdGenerator<std::string> m_namecache;
#endif
};

#endif


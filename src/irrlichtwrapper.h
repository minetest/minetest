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

#include <jmutex.h>
#include <jmutexautolock.h>
#include <string>

/*
	A thread-safe texture pointer cache.
	
	This is used so that irrlicht doesn't get called from many
	threads, because texture pointers have to be handled in
	background threads.
*/

class TextureCache
{
public:
	TextureCache()
	{
		m_mutex.Init();
		assert(m_mutex.IsInitialized());
	}
	
	void set(std::string name, video::ITexture *texture)
	{
		if(texture == NULL)
			return;
		
		JMutexAutoLock lock(m_mutex);

		m_textures[name] = texture;
	}
	
	video::ITexture* get(std::string name)
	{
		JMutexAutoLock lock(m_mutex);

		core::map<std::string, video::ITexture*>::Node *n;
		n = m_textures.find(name);

		if(n != NULL)
			return n->getValue();

		return NULL;
	}

private:
	core::map<std::string, video::ITexture*> m_textures;
	JMutex m_mutex;
};

struct TextureMod
{
	/*
		Returns a new texture which can be based on the original.
		Shall not modify or delete the original texture.
	*/
	virtual video::ITexture * make(video::ITexture *original,
			const char *newname, video::IVideoDriver* driver) = 0;
};

struct CrackTextureMod: public TextureMod
{
	CrackTextureMod(u16 a_progression)
	{
		progression = a_progression;
	}
	
	virtual video::ITexture * make(video::ITexture *original,
			const char *newname, video::IVideoDriver* driver);
	
	u16 progression;
};

struct SideGrassTextureMod: public TextureMod
{
	SideGrassTextureMod()
	{
	}
	
	virtual video::ITexture * make(video::ITexture *original,
			const char *newname, video::IVideoDriver* driver);
};

struct ProgressBarTextureMod: public TextureMod
{
	// value is from 0.0 to 1.0
	ProgressBarTextureMod(float a_value)
	{
		value = a_value;
	}
	
	virtual video::ITexture * make(video::ITexture *original,
			const char *newname, video::IVideoDriver* driver);
	
	float value;
};

/*
	A class for specifying a requested texture
*/
struct TextureSpec
{
	TextureSpec()
	{
		mod = NULL;
	}
	TextureSpec(const std::string &a_name, const std::string &a_path,
			TextureMod *a_mod)
	{
		name = a_name;
		path = a_path;
		mod = a_mod;;
	}
	~TextureSpec()
	{
	}
	bool operator==(const TextureSpec &other)
	{
		return name == other.name;
	}
	// An unique name for the texture. Usually the same as the path.
	// Note that names and paths reside the same namespace.
	std::string name;
	// This is the path of the base texture
	std::string path;
	// Modification to do to the base texture
	// NOTE: This is deleted by the one who processes the request
	TextureMod *mod;
};

/*
	A thread-safe wrapper for irrlicht, to be accessed from
	background worker threads.

	Queues tasks to be done in the main thread.
*/

class IrrlichtWrapper
{
public:
	/*
		These are called from the main thread
	*/
	IrrlichtWrapper(IrrlichtDevice *device);
	
	// Run queued tasks
	void Run();

	/*
		These are called from other threads
	*/

	// Not exactly thread-safe but this needs to be fast.
	// getTimer()->getRealTime() only reads one variable anyway.
	u32 getTime()
	{
		return m_device->getTimer()->getRealTime();
	}
	
	video::ITexture* getTexture(TextureSpec spec);
	video::ITexture* getTexture(const std::string &path);

private:
	/*
		Non-thread-safe variants of stuff, for internal use
	*/
	video::ITexture* getTextureDirect(TextureSpec spec);
	
	/*
		Members
	*/
	
	threadid_t m_main_thread;

	JMutex m_device_mutex;
	IrrlichtDevice *m_device;

	TextureCache m_texturecache;
	
	RequestQueue<TextureSpec, video::ITexture*, u8, u8> m_get_texture_queue;
};

#endif


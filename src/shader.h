/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Kahrl <kahrl@gmx.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include <IMaterialRendererServices.h>
#include "irrlichttypes_bloated.h"
#include <string>

class IGameDef;

/*
	shader.{h,cpp}: Shader handling stuff.
*/

/*
	Gets the path to a shader by first checking if the file
	  name_of_shader/filename
	exists in shader_path and if not, using the data path.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getShaderPath(const std::string &name_of_shader,
		const std::string &filename);

struct ShaderInfo {
	std::string name = "";
	video::E_MATERIAL_TYPE base_material = video::EMT_SOLID;
	video::E_MATERIAL_TYPE material = video::EMT_SOLID;
	u8 drawtype = 0;
	u8 material_type = 0;

	ShaderInfo() = default;
	virtual ~ShaderInfo() = default;
};

/*
	Setter of constants for shaders
*/

namespace irr { namespace video {
	class IMaterialRendererServices;
} }


class IShaderConstantSetter {
public:
	virtual ~IShaderConstantSetter() = default;
	virtual void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel) = 0;
};


class IShaderConstantSetterFactory {
public:
	virtual ~IShaderConstantSetterFactory() = default;
	virtual IShaderConstantSetter* create() = 0;
};


template <typename T, std::size_t count=1>
class CachedShaderSetting {
	const char *m_name;
	T m_sent[count];
	bool has_been_set = false;
	bool is_pixel;
protected:
	CachedShaderSetting(const char *name, bool is_pixel) :
		m_name(name), is_pixel(is_pixel)
	{}
public:
	void set(const T value[count], video::IMaterialRendererServices *services)
	{
		if (has_been_set && std::equal(m_sent, m_sent + count, value))
			return;
		if (is_pixel)
			services->setPixelShaderConstant(m_name, value, count);
		else
			services->setVertexShaderConstant(m_name, value, count);
		std::copy(value, value + count, m_sent);
		has_been_set = true;
	}
};

template <typename T, std::size_t count = 1>
class CachedPixelShaderSetting : public CachedShaderSetting<T, count> {
public:
	CachedPixelShaderSetting(const char *name) :
		CachedShaderSetting<T, count>(name, true){}
};

template <typename T, std::size_t count = 1>
class CachedVertexShaderSetting : public CachedShaderSetting<T, count> {
public:
	CachedVertexShaderSetting(const char *name) :
		CachedShaderSetting<T, count>(name, false){}
};


/*
	ShaderSource creates and caches shaders.
*/

class IShaderSource {
public:
	IShaderSource() = default;
	virtual ~IShaderSource() = default;

	virtual u32 getShaderIdDirect(const std::string &name,
		const u8 material_type, const u8 drawtype){return 0;}
	virtual ShaderInfo getShaderInfo(u32 id){return ShaderInfo();}
	virtual u32 getShader(const std::string &name,
		const u8 material_type, const u8 drawtype){return 0;}
};

class IWritableShaderSource : public IShaderSource {
public:
	IWritableShaderSource() = default;
	virtual ~IWritableShaderSource() = default;

	virtual u32 getShaderIdDirect(const std::string &name,
		const u8 material_type, const u8 drawtype){return 0;}
	virtual ShaderInfo getShaderInfo(u32 id){return ShaderInfo();}
	virtual u32 getShader(const std::string &name,
		const u8 material_type, const u8 drawtype){return 0;}

	virtual void processQueue()=0;
	virtual void insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program)=0;
	virtual void rebuildShaders()=0;
	virtual void addShaderConstantSetterFactory(IShaderConstantSetterFactory *setter) = 0;
};

IWritableShaderSource *createShaderSource();

void dumpShaderProgram(std::ostream &output_stream,
	const std::string &program_type, const std::string &program);

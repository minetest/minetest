// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
// Copyright (C) 2013 Kahrl <kahrl@gmx.net>

#pragma once

#include <IMaterialRendererServices.h>
#include <string>
#include "nodedef.h"

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
	NodeDrawType drawtype = NDT_NORMAL;
	MaterialType material_type = TILE_MATERIAL_BASIC;

	ShaderInfo() = default;
	virtual ~ShaderInfo() = default;
};

/*
	Setter of constants for shaders
*/

namespace irr::video {
	class IMaterialRendererServices;
}


class IShaderConstantSetter {
public:
	virtual ~IShaderConstantSetter() = default;
	virtual void onSetConstants(video::IMaterialRendererServices *services) = 0;
	virtual void onSetMaterial(const video::SMaterial& material)
	{ }
};


class IShaderConstantSetterFactory {
public:
	virtual ~IShaderConstantSetterFactory() = default;
	virtual IShaderConstantSetter* create() = 0;
};


template <typename T, std::size_t count, bool cache>
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
		if (cache && has_been_set && std::equal(m_sent, m_sent + count, value))
			return;
		if (is_pixel)
			services->setPixelShaderConstant(services->getPixelShaderConstantID(m_name), value, count);
		else
			services->setVertexShaderConstant(services->getVertexShaderConstantID(m_name), value, count);

		if (cache) {
			std::copy(value, value + count, m_sent);
			has_been_set = true;
		}
	}

	/* Type specializations */

	/*
	 * T2 looks redundant here but it is necessary so the compiler won't
	 * resolve the templates at class instantiation and then fail because
	 * some of these methods don't have valid types (= are not usable).
	 * ref: <https://stackoverflow.com/a/6972771>
	 *
	 * Note: a `bool dummy` template parameter would have been easier but MSVC
	 * does not like that. Also make sure not to define different specializations
	 * with the same parameters, MSVC doesn't like that either.
	 * I extend my thanks to Microsoft®
	 */
#define SPECIALIZE(_type, _count_expr) \
	template<typename T2 = T> \
	std::enable_if_t<std::is_same_v<T, T2> && std::is_same_v<T2, _type> && (_count_expr)>

	SPECIALIZE(float, count == 2)
	set(const v2f value, video::IMaterialRendererServices *services)
	{
		float array[2] = { value.X, value.Y };
		set(array, services);
	}

	SPECIALIZE(float, count == 3)
	set(const v3f value, video::IMaterialRendererServices *services)
	{
		float array[3] = { value.X, value.Y, value.Z };
		set(array, services);
	}

	SPECIALIZE(float, count == 3 || count == 4)
	set(const video::SColorf value, video::IMaterialRendererServices *services)
	{
		if constexpr (count == 3) {
			float array[3] = { value.r, value.g, value.b };
			set(array, services);
		} else {
			float array[4] = { value.r, value.g, value.b, value.a };
			set(array, services);
		}
	}

	SPECIALIZE(float, count == 16)
	set(const core::matrix4 &value, video::IMaterialRendererServices *services)
	{
		set(value.pointer(), services);
	}

#undef SPECIALIZE
};

template <typename T, std::size_t count = 1, bool cache=true>
class CachedPixelShaderSetting : public CachedShaderSetting<T, count, cache> {
public:
	CachedPixelShaderSetting(const char *name) :
		CachedShaderSetting<T, count, cache>(name, true){}
};

template <typename T, std::size_t count = 1, bool cache=true>
class CachedVertexShaderSetting : public CachedShaderSetting<T, count, cache> {
public:
	CachedVertexShaderSetting(const char *name) :
		CachedShaderSetting<T, count, cache>(name, false){}
};

template <typename T, std::size_t count, bool cache, bool is_pixel>
class CachedStructShaderSetting {
	const char *m_name;
	T m_sent[count];
	bool has_been_set = false;
	std::array<const char*, count> m_fields;
public:
	CachedStructShaderSetting(const char *name, std::array<const char*, count> &&fields) :
		m_name(name), m_fields(std::move(fields))
	{}

	void set(const T value[count], video::IMaterialRendererServices *services)
	{
		if (cache && has_been_set && std::equal(m_sent, m_sent + count, value))
			return;

		for (std::size_t i = 0; i < count; i++) {
			std::string uniform_name = std::string(m_name) + "." + m_fields[i];

			if (is_pixel)
				services->setPixelShaderConstant(services->getPixelShaderConstantID(uniform_name.c_str()), value + i, 1);
			else
				services->setVertexShaderConstant(services->getVertexShaderConstantID(uniform_name.c_str()), value + i, 1);
		}

		if (cache) {
			std::copy(value, value + count, m_sent);
			has_been_set = true;
		}
	}
};

template<typename T, std::size_t count, bool cache = true>
using CachedStructVertexShaderSetting = CachedStructShaderSetting<T, count, cache, false>;

template<typename T, std::size_t count, bool cache = true>
using CachedStructPixelShaderSetting = CachedStructShaderSetting<T, count, cache, true>;

/*
	ShaderSource creates and caches shaders.

	A "shader" could more precisely be called a "shader material" and comprises
	a vertex, fragment and optional geometry shader.
*/
class IShaderSource {
public:
	IShaderSource() = default;
	virtual ~IShaderSource() = default;

	/**
	 * @brief returns information about an existing shader
	 *
	 * Use this to get the material ID to plug into `video::SMaterial`.
	 */
	virtual ShaderInfo getShaderInfo(u32 id) = 0;

	/// @brief Generates or gets a shader suitable for nodes and entities
	virtual u32 getShader(const std::string &name,
		MaterialType material_type, NodeDrawType drawtype = NDT_NORMAL) = 0;

	/**
	 * Generates or gets a shader for general use.
	 * @param name name of the shader (directory on disk)
	 * @param blendAlpha enable alpha blending for this material?
	 * @return shader ID
	 */
	virtual u32 getShaderRaw(const std::string &name, bool blendAlpha = false) = 0;
};

class IWritableShaderSource : public IShaderSource {
public:
	IWritableShaderSource() = default;
	virtual ~IWritableShaderSource() = default;

	virtual void processQueue()=0;
	virtual void insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program)=0;
	virtual void rebuildShaders()=0;

	/// @note Takes ownership of @p setter.
	virtual void addShaderConstantSetterFactory(IShaderConstantSetterFactory *setter) = 0;
};

IWritableShaderSource *createShaderSource();

void dumpShaderProgram(std::ostream &output_stream,
	const std::string &program_type, std::string_view program);

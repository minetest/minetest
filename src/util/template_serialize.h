/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef UTIL_TEMPLATE_SERIALIZE_HEADER
#define UTIL_TEMPLATE_SERIALIZE_HEADER

#include "serialize.h"

#include <map>
#include <vector>

/*
	Templated serialization
*/

#define VALUE_TYPE_UNDEFINED 0
#define VALUE_TYPE_CUSTOM 1
#define VALUE_TYPE_RAW 2
#define VALUE_TYPE_BOOL 3
#define VALUE_TYPE_U8 4
#define VALUE_TYPE_U16 5
#define VALUE_TYPE_U32 6
#define VALUE_TYPE_S8 7
#define VALUE_TYPE_S16 8
#define VALUE_TYPE_S32 9
#define VALUE_TYPE_F1000 10
//#define VALUE_TYPE_F32 11
#define VALUE_TYPE_ARGB8 12

template<typename T>
struct STraits
{
};

template<>
struct STraits<std::vector<u8> >
{
	static u8 type()
	{
		return VALUE_TYPE_RAW;
	}
	static bool read(const std::vector<u8> &src, std::vector<u8> *result)
	{
		if(result)
			*result = src;
		return true;
	}
	static void write(const std::vector<u8> &src, std::vector<u8> &result)
	{
		result = src;
	}
};

template<>
struct STraits<SharedBuffer<u8> >
{
	static u8 type()
	{
		return VALUE_TYPE_RAW;
	}
	static bool read(const std::vector<u8> &src, SharedBuffer<u8> *result)
	{
		if(result)
			*result = SharedBuffer<u8>(&src[0], src.size());
		return true;
	}
	static void write(const SharedBuffer<u8> &src, std::vector<u8> &result)
	{
		result.insert(result.begin(), &src[0], &src[0] + src.getSize());
	}
};

template<>
struct STraits<std::string>
{
	static u8 type()
	{
		return VALUE_TYPE_RAW;
	}
	static bool read(const std::vector<u8> &src, std::string *result)
	{
		if(result)
			*result = std::string((const char*)&src[0], src.size());
		return true;
	}
	static void write(const std::string &src, std::vector<u8> &result)
	{
		result.insert(result.begin(), (u8*)src.c_str(),
				(u8*)src.c_str() + src.size());
	}
};

template<>
struct STraits<bool>
{
	static u8 type()
	{
		return VALUE_TYPE_BOOL;
	}
	static bool read(const std::vector<u8> &src, bool *result)
	{
		if(src.size() != 1)
			return false;
		if(result)
			*result = !!src[0];
		return true;
	}
	static void write(const bool &src, std::vector<u8> &result)
	{
		result.push_back(src);
	}
};

template<>
struct STraits<u8>
{
	static u8 type()
	{
		return VALUE_TYPE_U8;
	}
	static bool read(const std::vector<u8> &src, u8 *result)
	{
		if(src.size() != 1)
			return false;
		if(result)
			*result = src[0];
		return true;
	}
	static void write(const u8 &src, std::vector<u8> &result)
	{
		result.push_back(src);
	}
};

template<>
struct STraits<u16>
{
	static u8 type()
	{
		return VALUE_TYPE_U16;
	}
	static bool read(const std::vector<u8> &src, u16 *result)
	{
		if(src.size() != 2)
			return false;
		if(result)
			*result = readU16(&src[0]);
		return true;
	}
	static void write(const u16 &src, std::vector<u8> &result)
	{
		result.resize(2);
		writeU16(&result[0], src);
	}
};

template<>
struct STraits<u32>
{
	static u8 type()
	{
		return VALUE_TYPE_U32;
	}
	static bool read(const std::vector<u8> &src, u32 *result)
	{
		if(src.size() != 4)
			return false;
		if(result)
			*result = readU32(&src[0]);
		return true;
	}
	static void write(const u32 &src, std::vector<u8> &result)
	{
		result.resize(4);
		writeU32(&result[0], src);
	}
};

template<>
struct STraits<s8>
{
	static u8 type()
	{
		return VALUE_TYPE_S8;
	}
	static bool read(const std::vector<s8> &src, s8 *result)
	{
		if(src.size() != 1)
			return false;
		if(result)
			*result = src[0];
		return true;
	}
	static void write(const s8 &src, std::vector<s8> &result)
	{
		result.push_back((u8)src);
	}
};

template<>
struct STraits<s16>
{
	static u8 type()
	{
		return VALUE_TYPE_S16;
	}
	static bool read(const std::vector<u8> &src, s16 *result)
	{
		if(src.size() != 2)
			return false;
		if(result)
			*result = readS16(&src[0]);
		return true;
	}
	static void write(const s16 &src, std::vector<u8> &result)
	{
		result.resize(2);
		writeS16(&result[0], src);
	}
};

template<>
struct STraits<s32>
{
	static u8 type()
	{
		return VALUE_TYPE_S32;
	}
	static bool read(const std::vector<u8> &src, s32 *result)
	{
		if(src.size() != 4)
			return false;
		if(result)
			*result = readS32(&src[0]);
		return true;
	}
	static void write(const s32 &src, std::vector<u8> &result)
	{
		result.resize(4);
		writeS32(&result[0], src);
	}
};

/*template<>
struct STraits<f32>
{
	static u8 type()
	{
		return VALUE_TYPE_F32;
	}
	static bool read(const std::vector<u8> &src, f32 *result)
	{
		if(src.size() != 4)
			return false;
		if(result)
			*result = readF32(&src[0]);
		return true;
	}
	static void write(const f32 &src, std::vector<u8> &result)
	{
		result.resize(4);
		writeF32(&result[0], src);
	}
};*/

struct F1000
{
	float v;

	F1000(float v):
		v(v)
	{}
};
template<>
struct STraits<F1000>
{
	static u8 type()
	{
		return VALUE_TYPE_F1000;
	}
	static bool read(const std::vector<u8> &src, F1000 *result)
	{
		if(src.size() != 4)
			return false;
		if(result)
			result->v = readF1000(&src[0]);
		return true;
	}
	static void write(const F1000 &src, std::vector<u8> &result)
	{
		result.resize(4);
		writeF1000(&result[0], src.v);
	}
};

template<>
struct STraits<video::SColor>
{
	static u8 type()
	{
		return VALUE_TYPE_ARGB8;
	}
	static bool read(const std::vector<u8> &src, video::SColor *result)
	{
		if(src.size() != 4)
			return false;
		if(result)
			*result = readARGB8(&src[0]);
		return true;
	}
	static void write(const video::SColor &src, std::vector<u8> &result)
	{
		result.resize(4);
		writeARGB8(&result[0], src);
	}
};

/*
	BinaryKeyValueList
	Stores and serializes stuff as a (numeric key) - (value list) store.
	All reads use a reference for the result and return a boolean (false=fail)
	The result is never modified if the value does not exist.
*/
class BinaryKeyValueList
{
	struct Value
	{
		std::vector<u8> v;
		u8 t;

		Value():
			t(VALUE_TYPE_UNDEFINED)
		{}
		Value(u8 t, u32 size):
			v(size),
			t(t)
		{}
		Value(u8 t, std::vector<u8> data):
			t(t)
		{
			v.insert(v.begin(), data.begin(), data.end());
		}
	};
	std::map<u32, std::vector<Value> > m_values;

	u32 priv_valueCount(u32 key) const
	{
		std::map<u32, std::vector<Value> >::const_iterator it = m_values.find(key);
		if(it == m_values.end())
			return 0;
		const std::vector<Value> &list = it->second;
		return list.size();
	}
	const Value* priv_accessRaw(u32 key, u32 i) const
	{
		std::map<u32, std::vector<Value> >::const_iterator it = m_values.find(key);
		if(it == m_values.end())
			return NULL;
		const std::vector<Value> &list = it->second;
		if(i >= list.size())
			return NULL;
		return &list[i];
	}
	Value* priv_accessRaw(u32 key, u32 i)
	{
		std::map<u32, std::vector<Value> >::iterator it = m_values.find(key);
		if(it == m_values.end())
			return NULL;
		std::vector<Value> &list = it->second;
		if(i >= list.size())
			return NULL;
		return &list[i];
	}
	void priv_clear()
	{
		m_values.clear();
	}
	void priv_clearKey(u32 key)
	{
		m_values.erase(key);
	}
	void priv_appendRaw(u32 key, const Value &data)
	{
		std::map<u32, std::vector<Value> >::iterator it = m_values.find(key);
		if(it == m_values.end()){
			m_values[key] = std::vector<Value>();
			it = m_values.find(key);
		}
		std::vector<Value> &list = it->second;
		list.push_back(data);
	}
	#define BS_ACCESS_OR_RETURN_FALSE(raw, key, i)\
		Value &raw = *priv_accessRaw(key, i);\
		if(!(&raw)) return false;
	#define BS_GET_OR_RETURN_FALSE(raw, key, i)\
		const Value &raw = *priv_accessRaw(key, i);\
		if(!(&raw)) return false;
public:

	/* Serialization */
	void serialize(std::ostream &os)
	{
		// Write version
		writeU8(os, 1);
		// Write data
		writeU32(os, m_values.size());
		for(std::map<u32, std::vector<Value> >::iterator
				it = m_values.begin(); it != m_values.end(); ++it)
		{
			u32 key = it->first;
			std::vector<Value> &list = it->second;
			bool size_written = false;
			/* Write key */
			if(key < 126){
				u8 v = key;
				if(list.size() == 1){
					v |= 0x80;
					size_written = true;
				}
				writeU8(os, v);
			} else if(key < 65536){
				u8 v = 126;
				if(list.size() == 1){
					v |= 0x80;
					size_written = true;
				}
				writeU8(os, v);
				writeU16(os, key);
			} else{
				u8 v = 127;
				if(list.size() == 1){
					v |= 0x80;
					size_written = true;
				}
				writeU8(os, v);
				writeU32(os, key);
			}
			/* Write values */
			if(!size_written){
				if(list.size() < 255){
					writeU8(os, list.size());
				} else{
					writeU8(os, 255);
					writeU32(os, list.size());
				}
			}
			for(size_t i=0; i<list.size(); i++){
				writeU8(os, list[i].t);
				if(list[i].v.size() < 255){
					writeU8(os, list[i].v.size());
				} else{
					writeU8(os, 255);
					writeU32(os, list[i].v.size());
				}
				os.write((const char*)&list[i].v[0], list[i].v.size());
			}
		}
	}
	void deSerialize(std::istream &is)
	{
		m_values.clear();
		// Read version
		u8 version = readU8(is);
		if(version != 1)
			throw SerializationError("Invalid BinaryKeyValueList version");
		// Read data
		u32 num_keys = readU32(is);
		for(u32 i=0; i<num_keys; i++){
			if(!is.good())
				throw SerializationError("BinaryKeyValueList::deSerialize(): "
						"Truncated input");
			/* Read key */
			u8 key_v = readU8(is);
			bool single_value = !!(key_v & 0x80);
			u32 key = key_v & 0x7f;
			if(key == 126)
				key = readU16(is);
			else if(key == 127)
				key = readU32(is);
			/* Get list to deserialize into */
			std::map<u32, std::vector<Value> >::iterator it = m_values.find(key);
			if(it == m_values.end()){
				m_values[key] = std::vector<Value>();
				it = m_values.find(key);
			}
			std::vector<Value> &list = it->second;
			/* Read values */
			u32 num_values = 1;
			if(!single_value){
				num_values = readU8(is);
				if(num_values == 255)
					num_values = readU32(is);
			}
			for(u32 i=0; i<num_values; i++){
				u8 value_type = readU8(is);
				u32 value_size = readU8(is);
				if(value_size == 255)
					value_size = readU32(is);
				Value value(value_type, value_size);
				is.read((char*)&value.v[0], value_size);
				list.push_back(value);
			}
		}
	}

	/* Misc. */

	void clear()
	{
		priv_clear();
	}

	void clearKey(u32 key)
	{
		priv_clearKey(key);
	}

	u32 valueCount(u32 key) const
	{
		return priv_valueCount(key);
	}

	/* Appenders */

	void appendRaw(u32 key, const Value &data)
	{
		priv_appendRaw(key, data);
	}
	void append(u32 key, const u8 *data, size_t len)
	{
		std::vector<u8> a;
		a.insert(a.end(), data, data+len);
		appendRaw(key, Value(VALUE_TYPE_RAW, a));
	}

	// To enable new types, define an STraits<type> for them.
	template<typename T>
	void append(u32 key, const T &value)
	{
		std::vector<u8> a;
		STraits<T>::write(value, a);
		appendRaw(key, Value(STraits<T>::type(), a));
	}

	/* Getters */

	bool getRaw(u32 key, u32 i, Value *result) const
	{
		BS_GET_OR_RETURN_FALSE(raw, key, i)
		if(result)
			*result = raw;
		return true;
	}

	// To enable new types, define an STraits<type> for them.
	template<typename T>
	bool get(u32 key, u32 i, T *result) const
	{
		BS_GET_OR_RETURN_FALSE(raw, key, i)
		if(STraits<T>::type() != raw.t)
			return false;
		return STraits<T>::read(raw.v, result);
	}
	template<typename T>
	bool get(u32 key, u32 i, T &result) const
	{
		return get(key, i, &result);
	}
	template<typename T>
	T getDirect(u32 key, u32 i, const T &default_value) const
	{
		T result = default_value;
		get(key, i, result);
		return result;
	}
};
typedef BinaryKeyValueList BKVL;

#endif

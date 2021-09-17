/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "irrlichttypes.h"
#include "debug.h" // For assert()
#include <cstring>

template <typename T>
class Buffer
{
public:
	Buffer()
	{
		m_size = 0;
		data = NULL;
	}
	Buffer(unsigned int size)
	{
		m_size = size;
		if(size != 0)
			data = new T[size];
		else
			data = NULL;
	}
	Buffer(const Buffer &buffer)
	{
		m_size = buffer.m_size;
		if(m_size != 0)
		{
			data = new T[buffer.m_size];
			memcpy(data, buffer.data, buffer.m_size);
		}
		else
			data = NULL;
	}
	Buffer(Buffer &&buffer)
	{
		m_size = buffer.m_size;
		if(m_size != 0)
		{
			data = buffer.data;
			buffer.data = nullptr;
			buffer.m_size = 0;
		}
		else
			data = nullptr;
	}
	// Copies whole buffer
	Buffer(const T *t, unsigned int size)
	{
		m_size = size;
		if(size != 0)
		{
			data = new T[size];
			memcpy(data, t, size);
		}
		else
			data = NULL;
	}

	~Buffer()
	{
		drop();
	}

	Buffer& operator=(const Buffer &buffer)
	{
		if(this == &buffer)
			return *this;
		drop();
		m_size = buffer.m_size;
		if(m_size != 0)
		{
			data = new T[buffer.m_size];
			memcpy(data, buffer.data, buffer.m_size);
		}
		else
			data = NULL;
		return *this;
	}
	Buffer& operator=(Buffer &&buffer)
	{
		if(this == &buffer)
			return *this;
		drop();
		m_size = buffer.m_size;
		if(m_size != 0)
		{
			data = buffer.data;
			buffer.data = nullptr;
			buffer.m_size = 0;
		}
		else
			data = nullptr;
		return *this;
	}

	T & operator[](unsigned int i) const
	{
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}

	unsigned int getSize() const
	{
		return m_size;
	}

private:
	void drop()
	{
		delete[] data;
	}
	T *data;
	unsigned int m_size;
};

/************************************************
 *           !!!  W A R N I N G  !!!            *
 *                                              *
 * This smart pointer class is NOT thread safe. *
 * ONLY use in a single-threaded context!       *
 *                                              *
 ************************************************/
template <typename T>
class SharedBuffer
{
public:
	SharedBuffer()
	{
		m_size = 0;
		data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	SharedBuffer(unsigned int size)
	{
		m_size = size;
		if(m_size != 0)
			data = new T[m_size];
		else
			data = NULL;
		refcount = new unsigned int;
		memset(data,0,sizeof(T)*m_size);
		(*refcount) = 1;
	}
	SharedBuffer(const SharedBuffer &buffer)
	{
		m_size = buffer.m_size;
		data = buffer.data;
		refcount = buffer.refcount;
		(*refcount)++;
	}
	SharedBuffer & operator=(const SharedBuffer & buffer)
	{
		if(this == &buffer)
			return *this;
		drop();
		m_size = buffer.m_size;
		data = buffer.data;
		refcount = buffer.refcount;
		(*refcount)++;
		return *this;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const T *t, unsigned int size)
	{
		m_size = size;
		if(m_size != 0)
		{
			data = new T[m_size];
			memcpy(data, t, m_size);
		}
		else
			data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const Buffer<T> &buffer)
	{
		m_size = buffer.getSize();
		if (m_size != 0) {
				data = new T[m_size];
				memcpy(data, *buffer, buffer.getSize());
		}
		else
			data = NULL;
		refcount = new unsigned int;
		(*refcount) = 1;
	}
	~SharedBuffer()
	{
		drop();
	}
	T & operator[](unsigned int i) const
	{
		assert(i < m_size);
		return data[i];
	}
	T * operator*() const
	{
		return data;
	}
	unsigned int getSize() const
	{
		return m_size;
	}
	operator Buffer<T>() const
	{
		return Buffer<T>(data, m_size);
	}
private:
	void drop()
	{
		assert((*refcount) > 0);
		(*refcount)--;
		if(*refcount == 0)
		{
			delete[] data;
			delete refcount;
		}
	}
	T *data;
	unsigned int m_size;
	unsigned int *refcount;
};

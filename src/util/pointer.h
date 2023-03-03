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
#include <memory>
#include <type_traits>


template <typename T>
struct is_unbounded_array : std::false_type {};
template <typename T>
struct is_unbounded_array<T[]> : std::true_type {};

template <typename T, std::enable_if_t<!std::is_array<T>::value, bool> = true>
std::unique_ptr<T> make_unique_for_overwrite()
{
	return std::unique_ptr<T>(new T);
}
template <typename T, std::enable_if_t<is_unbounded_array<T>::value, bool> = true>
std::unique_ptr<T> make_unique_for_overwrite(size_t size)
{
	return std::unique_ptr<T>(new std::remove_extent_t<T>[size]);
}


//! A std::unique_ptr<T[]> with size. Like std::vector<T>, but not growable.
template <typename T>
class UniqueBuffer
{
public:
	constexpr UniqueBuffer() noexcept = default;
	constexpr UniqueBuffer(std::nullptr_t) noexcept {}

	constexpr UniqueBuffer(std::unique_ptr<T[]> data, size_t size) noexcept :
		m_data(std::move(data)), m_size(size) {}

	UniqueBuffer(const UniqueBuffer &) = delete;

	constexpr UniqueBuffer(UniqueBuffer &&other) noexcept :
		m_data(std::move(other.m_data)), m_size(other.m_size) {}

	~UniqueBuffer() = default;

	constexpr UniqueBuffer &operator=(const UniqueBuffer &) = delete;

	constexpr UniqueBuffer &operator=(UniqueBuffer &&other) noexcept
	{
		if (&other == this)
			return *this;
		m_data = std::move(other.m_data);
		m_size = other.m_size;
		other.m_size = 0;
		return *this;
	}

	constexpr UniqueBuffer &operator=(std::nullptr_t) noexcept
	{
		m_data = nullptr;
		m_size = 0;
	}

	constexpr std::unique_ptr<T[]> release() noexcept
	{
		m_size = 0;
		return std::move(m_data);
	}

	constexpr void reset(std::unique_ptr<T[]> data, size_t size) noexcept
	{
		m_data = data;
		m_size = size;
	}

	constexpr void reset(std::nullptr_t) noexcept
	{
		m_size = 0;
		m_data = nullptr;
	}

	constexpr void swap(UniqueBuffer &other) noexcept
	{
		std::swap(m_size, other.m_size);
		std::swap(m_data, other.m_data);
	}

	constexpr T *get() const noexcept { return m_data.get(); }

	explicit constexpr operator bool() const noexcept { return (bool)m_data; }

	constexpr T &operator[](size_t i) const { return m_data[i]; }

	void copyTo(UniqueBuffer &other) const
	{
		if (other.m_size != m_size)
			other = UniqueBuffer(make_unique_for_overwrite<T[]>(m_size), m_size);

		for (size_t i = 0; i != m_size; ++i)
			other.m_data[i] = m_data[i];
	}

	UniqueBuffer copy() const
	{
		UniqueBuffer ret = UniqueBuffer(make_unique_for_overwrite<T[]>(m_size), m_size);
		for (size_t i = 0; i != m_size; ++i)
			ret.m_data[i] = m_data[i];
		return ret;
	}

	constexpr size_t size() const noexcept { return m_size; }

	static UniqueBuffer make(size_t size)
	{
		return size == 0 ? UniqueBuffer() :
				UniqueBuffer(std::make_unique<T[]>(size), size);
	}

	static UniqueBuffer makeForOverwrite(size_t size)
	{
		return size == 0 ? UniqueBuffer() :
				UniqueBuffer(make_unique_for_overwrite<T[]>(size), size);
	}

private:
	std::unique_ptr<T[]> m_data = nullptr;
	size_t m_size = 0;
};

template <typename T>
UniqueBuffer<T> copy_to_unique_buffer(const T *data, size_t size)
{
	auto ret = UniqueBuffer<T>::makeForOverwrite(size);
	for (size_t i = 0; i != size; ++i)
		ret[i] = data[i];
	return ret;
}

template <typename T>
constexpr void swap(UniqueBuffer<T> &a, UniqueBuffer<T> &b) noexcept
{
	a.swap(b);
}

template <typename T, typename U>
constexpr bool operator==(const UniqueBuffer<T> &a, const UniqueBuffer<U> &b) noexcept
{
	return a.get() == b.get();
}
template <typename T, typename U>
constexpr bool operator!=(const UniqueBuffer<T> &a, const UniqueBuffer<U> &b) noexcept
{
	return a.get() != b.get();
}
template <typename T>
constexpr bool operator==(const UniqueBuffer<T> &a, std::nullptr_t) noexcept
{
	return (bool)a;
}
template <typename T>
constexpr bool operator==(std::nullptr_t, const UniqueBuffer<T> &a) noexcept
{
	return (bool)a;
}
template <typename T>
constexpr bool operator!=(const UniqueBuffer<T> &a, std::nullptr_t) noexcept
{
	return !a;
}
template <typename T>
constexpr bool operator!=(std::nullptr_t, const UniqueBuffer<T> &a) noexcept
{
	return !a;
}


//! Similar to std::shared_ptr<T[]>, but with size.
//!
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
		m_data = nullptr;
		m_refcount = new unsigned int;
		(*m_refcount) = 1;
	}
	SharedBuffer(size_t size)
	{
		m_size = size;
		if (m_size != 0)
			m_data = new T[m_size];
		else
			m_data = nullptr;
		m_refcount = new unsigned int;
		memset(m_data, 0, sizeof(T) * m_size);
		(*m_refcount) = 1;
	}
	SharedBuffer(const SharedBuffer &buffer)
	{
		m_size = buffer.m_size;
		m_data = buffer.m_data;
		m_refcount = buffer.m_refcount;
		(*m_refcount)++;
	}
	SharedBuffer &operator=(const SharedBuffer &buffer)
	{
		if (this == &buffer)
			return *this;
		drop();
		m_size = buffer.m_size;
		m_data = buffer.m_data;
		m_refcount = buffer.m_refcount;
		(*m_refcount)++;
		return *this;
	}
	/*
		Copies whole buffer
	*/
	SharedBuffer(const T *t, size_t size)
	{
		m_size = size;
		if(m_size != 0)
		{
			m_data = new T[m_size];
			memcpy(m_data, t, m_size);
		}
		else
			m_data = nullptr;
		m_refcount = new unsigned int;
		(*m_refcount) = 1;
	}
	SharedBuffer(UniqueBuffer<T> &&buffer)
	{
		m_size = buffer.size();
		m_data = buffer.release().release();
		m_refcount = new unsigned int;
		(*m_refcount) = 1;
	}
	~SharedBuffer()
	{
		drop();
	}
	T &operator[](size_t i) const
	{
		assert(i < m_size);
		return m_data[i];
	}
	T *operator*() const
	{
		return m_data;
	}
	T *get() const
	{
		return m_data;
	}
	size_t size() const
	{
		return m_size;
	}
	UniqueBuffer<T> moveOut()
	{
		SANITY_CHECK(*m_refcount == 1);
		size_t size = m_size;
		auto data = std::unique_ptr<T>(m_data);
		m_size = 0;
		m_data = nullptr;
		return UniqueBuffer<T>(std::move(data), size);
	}
	UniqueBuffer<T> copy() const
	{
		auto ret = UniqueBuffer<T>::makeForOverwrite(m_size);
		for (size_t i = 0; i != m_size; ++i)
			ret[i] = m_data[i];
		return ret;
	}

private:
	void drop()
	{
		assert((*m_refcount) > 0);
		(*m_refcount)--;
		if (*m_refcount == 0) {
			delete[] m_data;
			delete m_refcount;
		}
	}
	T *m_data;
	size_t m_size;
	unsigned int *m_refcount;
};


// This class is not thread-safe!
class IntrusiveReferenceCounted {
public:
	IntrusiveReferenceCounted() = default;
	virtual ~IntrusiveReferenceCounted() = default;
	void grab() noexcept { ++m_refcount; }
	void drop() noexcept { if (--m_refcount == 0) delete this; }

	// Preserve own reference count.
	IntrusiveReferenceCounted(const IntrusiveReferenceCounted &) {}
	IntrusiveReferenceCounted &operator=(const IntrusiveReferenceCounted &) { return *this; }
private:
	u32 m_refcount = 1;
};

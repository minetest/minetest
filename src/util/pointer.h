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
#include <iterator>
#include <memory>
#include <sstream>
#include <type_traits>


template <typename T>
struct is_unbounded_array : std::false_type {};
template <typename T>
struct is_unbounded_array<T[]> : std::true_type {};

// Whether a `buffer<From>` can be implicitly casted to `buffer<To>`, where `buffer`
// is some buffer smart-ptr.
// We only allow to implicitly cast from non-const to const. If you need other
// conversions, cast the underlying ptrs.
template <typename From, typename To>
using is_implicitly_array_convertible = std::integral_constant<bool,
		std::is_same<From, To>::value
		|| std::is_same<From, std::remove_const_t<To>>::value
	>;

// Use this for default-initialization (aka don't fill with zeros)
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


/*
 * Buffer smart-ptrs
 * =================
 *
 * If you want to create a buffer, use make_buffer<T>() or make_buffer_for_overwrite<T>().
 * This creates a UniqueBuffer<T>, which is cheaply convertible to a SharedBuffer<T>.
 * Also, always prefer returning a UniqueBuffer<T> if the buffer hasn't multiple
 * owners.
 *
 * If you only want to read some buffer, use ConstView<T>. If you also need to
 * write, but don't need ownership, use a View<T>.
 *
 * The buffer smart-ptrs are small. Pass View<T>s by value.
 *
 * Note that const buffer smart-ptrs do not necessarily have const data, i.e.
 * `const UniqueBuffer<const T>` is similar to `const std::vector<T>`, not
 * `const UniqueBuffer<T>`.
 */


//! A std::unique_ptr<T[]> with size. Like std::vector<T>, but not growable.
template <typename T>
class UniqueBuffer
{
public:
	using value_type = T;
	using size_type = size_t;
	using reference = T &;
	using iterator = T *;
	using const_iterator = const T *;

	constexpr UniqueBuffer() noexcept = default;
	constexpr UniqueBuffer(std::nullptr_t) noexcept {}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr UniqueBuffer(std::unique_ptr<U[]> data, size_t size) noexcept :
		m_data(std::move(data)), m_size(size) {}

	UniqueBuffer(const UniqueBuffer &) = delete;

	constexpr UniqueBuffer(UniqueBuffer &&other) noexcept :
		m_size(other.size())
	{
		m_data = other.release();
	}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr UniqueBuffer(UniqueBuffer<U> &&other) noexcept :
		m_size(other.size())
	{
		m_data = other.release();
	}

	~UniqueBuffer() = default;

	constexpr UniqueBuffer &operator=(const UniqueBuffer &) = delete;

	constexpr UniqueBuffer &operator=(UniqueBuffer &&other) noexcept
	{
		if (&other == this)
			return *this;
		m_size = other.size();
		m_data = other.release();
		return *this;
	}

	constexpr UniqueBuffer &operator=(std::nullptr_t) noexcept
	{
		reset();
	}

	constexpr std::unique_ptr<T[]> release() noexcept
	{
		m_size = 0;
		return std::move(m_data);
	}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr void reset(std::unique_ptr<U[]> data, size_t size) noexcept
	{
		m_data = std::move(data);
		m_size = size;
	}

	constexpr void reset(std::nullptr_t = nullptr) noexcept
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

	constexpr T &operator[](size_t i) const noexcept { return m_data[i]; }

	T &at(size_t i) const
	{
		if (i >= m_size) {
			std::ostringstream os;
			os << "UniqueBuffer::at: i (" << i << ") >= m_size (" << m_size << ")";
			throw std::out_of_range(os.str());
		}
		return m_data[i];
	}

	template <typename U, std::enable_if_t<
			std::is_convertible<T, U>::value, bool> = true
	>
	void copyTo(UniqueBuffer<U> &other) const
	{
		if (other.size() != m_size)
			other.reset(make_unique_for_overwrite<U[]>(m_size), m_size);

		for (size_t i = 0; i != m_size; ++i)
			other[i] = m_data[i];
	}

	UniqueBuffer<std::remove_const_t<T>> copy() const;

	constexpr size_t size() const noexcept { return m_size; }

	constexpr size_t empty() const noexcept { return m_size == 0; }

private:
	std::unique_ptr<T[]> m_data = nullptr;
	size_t m_size = 0;
};

template <typename T>
constexpr void swap(UniqueBuffer<T> &a, UniqueBuffer<T> &b) noexcept
{
	a.swap(b);
}

template <typename T>
constexpr UniqueBuffer<T> make_buffer(size_t size)
{
	return size == 0 ? UniqueBuffer<T>() :
			UniqueBuffer<T>(std::make_unique<T[]>(size), size);
}

template <typename T>
constexpr UniqueBuffer<T> make_buffer_for_overwrite(size_t size)
{
	return size == 0 ? UniqueBuffer<T>() :
			UniqueBuffer<T>(make_unique_for_overwrite<T[]>(size), size);
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
	using value_type = T;
	using size_type = size_t;
	using reference = T &;
	using iterator = T *;
	using const_iterator = const T *;

	constexpr SharedBuffer() noexcept = default;

	constexpr SharedBuffer(std::nullptr_t) noexcept {}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	SharedBuffer(UniqueBuffer<U> &&buffer) :
		m_size(buffer.size()), m_refcount(nullptr)
	{
		m_data = buffer.release().release();
		if (m_data) {
			m_refcount = new unsigned int;
			*m_refcount = 1;
		}
	}

	constexpr SharedBuffer(SharedBuffer &&buffer) noexcept
	{
		swap(buffer);
	}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr SharedBuffer(SharedBuffer<U> &&buffer) noexcept :
		m_data(buffer.m_data), m_size(buffer.m_size), m_refcount(buffer.m_refcount)
	{
		buffer.m_data = nullptr;
		buffer.m_size = 0;
		buffer.m_refcount = nullptr;
	}

	constexpr SharedBuffer(const SharedBuffer &buffer) noexcept :
		m_data(buffer.m_data), m_size(buffer.m_size), m_refcount(buffer.m_refcount)
	{
		if (m_refcount)
			*m_refcount += 1;
	}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr SharedBuffer(const SharedBuffer<U> &buffer) noexcept :
		m_data(buffer.m_data), m_size(buffer.m_size), m_refcount(buffer.m_refcount)
	{
		if (m_refcount)
			*m_refcount += 1;
	}

	SharedBuffer &operator=(std::nullptr_t)
	{
		reset();
		return *this;
	}

	constexpr SharedBuffer &operator=(SharedBuffer &&buffer) noexcept
	{
		if (this == &buffer)
			return *this;
		reset();
		swap(buffer);
		return *this;
	}

	constexpr SharedBuffer &operator=(const SharedBuffer &buffer) noexcept
	{
		if (this == &buffer)
			return *this;
		return (*this = SharedBuffer(buffer));
	}

	~SharedBuffer()
	{
		drop();
	}

	constexpr void reset(std::nullptr_t = nullptr) noexcept
	{
		drop();
		m_data = nullptr;
		m_size = 0;
		m_refcount = nullptr;
	}

	constexpr void swap(SharedBuffer &buffer) noexcept
	{
		std::swap(m_data, buffer.m_data);
		std::swap(m_size, buffer.m_size);
		std::swap(m_refcount, buffer.m_refcount);
	}

	constexpr T &operator[](size_t i) const noexcept { return m_data[i]; }

	T &at(size_t i) const
	{
		if (i >= m_size) {
			std::ostringstream os;
			os << "SharedBuffer::at: i (" << i << ") >= m_size (" << m_size << ")";
			throw std::out_of_range(os.str());
		}
		return m_data[i];
	}

	constexpr T *get() const noexcept { return m_data; }

	constexpr size_t size() const noexcept { return m_size; }

	explicit constexpr operator bool() { return (bool)m_refcount; }

	constexpr size_t empty() const noexcept { return m_size == 0; }

	UniqueBuffer<T> moveOut()
	{
		UniqueBuffer<T> ret;
		if (!m_refcount || *m_refcount != 1)
			throw std::runtime_error("SharedBuffer::moveOut: Preconditions not met");
		size_t size = m_size;
		auto data = std::unique_ptr<T[]>(m_data);
		delete m_refcount;
		m_size = 0;
		m_data = nullptr;
		m_refcount = nullptr;
		return UniqueBuffer<T>(std::move(data), size);
	}

	UniqueBuffer<std::remove_const_t<T>> copy() const;

	UniqueBuffer<T> moveOrCopyOut()
	{
		if (!m_refcount) {
			return UniqueBuffer<T>();
		} else if (*m_refcount == 1) {
			return moveOut();
		} else {
			UniqueBuffer<T> ret = copy();
			reset();
			return ret;
		}
	}

private:
	void drop() noexcept
	{
		if (!m_refcount)
			return;
		assert((*m_refcount) > 0);
		*m_refcount -= 1;
		if (*m_refcount == 0) {
			delete m_refcount;
			delete[] m_data;
		}
	}

	friend SharedBuffer<std::remove_const_t<T>>;
	friend SharedBuffer<const std::remove_const_t<T>>;

	// values for an unset SharedBuffer:
	T *m_data = nullptr;
	size_t m_size = 0;
	unsigned int *m_refcount = nullptr;
};

template <typename T>
constexpr void swap(SharedBuffer<T> &a, SharedBuffer<T> &b)
{
	a.swap(b);
}


template <typename T>
class View
{
public:
	using value_type = T;
	using size_type = size_t;
	using reference = T &;
	using iterator = T *;
	using const_iterator = const T *;

	constexpr View() noexcept = default;

	constexpr View(std::nullptr_t) noexcept {}

	constexpr View(const View &) noexcept = default;
	constexpr View(View &&) noexcept = default;

	constexpr View &operator=(const View &) noexcept = default;
	constexpr View &operator=(View &&) noexcept = default;

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr View(View<U> other) noexcept :
			m_data(other.get()), m_size(other.size()) {}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr View(U *data, size_t size) noexcept :
		m_data(data), m_size(size) {}

	// `It` must also satisfy LegacyContiguousIterator.
	template <typename It, std::enable_if_t<
			is_implicitly_array_convertible<typename std::iterator_traits<It>::value_type, T>::value
			&& std::is_same<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>::value
		, bool> = true
	>
	constexpr View(It begin, It end) noexcept :
		m_data(&*begin), m_size(end - begin) {}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr View(const UniqueBuffer<U> &buffer) noexcept :
		m_data(buffer.get()), m_size(buffer.size()) {}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr View(const SharedBuffer<U> &buffer) noexcept :
		m_data(buffer.get()), m_size(buffer.size()) {}

	constexpr T *begin() const noexcept { return m_data; }
	constexpr T *end() const noexcept { return m_data + m_size; }

	constexpr const T *cbegin() const noexcept { return begin(); }
	constexpr const T *cend() const noexcept { return end(); }

	constexpr T *get() const noexcept { return m_data; }

	constexpr size_t size() const noexcept { return m_size; }

	// Returns whether the View is set to something (but it can be empty).
	explicit constexpr operator bool() const noexcept { return (bool)m_data; }

	constexpr bool empty() const noexcept { return m_size == 0; }

	constexpr T &operator[](size_t i) const noexcept { return m_data[i]; }

	T &at(size_t i) const
	{
		if (i >= m_size) {
			std::ostringstream os;
			os << "View::at: i (" << i << ") >= m_size (" << m_size << ")";
			throw std::out_of_range(os.str());
		}
		return m_data[i];
	}

	constexpr T &front() const noexcept { return m_data[0]; }
	constexpr T &back() const noexcept { return m_data[m_size - 1]; }

	constexpr void reset(std::nullptr_t = nullptr) noexcept
	{
		m_data = nullptr;
		m_size = 0;
	}

	template <typename U, std::enable_if_t<
			is_implicitly_array_convertible<U, T>::value, bool> = true
	>
	constexpr void reset(U *data, size_t size) noexcept
	{
		m_data = data;
		m_size = size;
	}

	// `It` must also satisfy LegacyContiguousIterator.
	template <typename It, std::enable_if_t<
			is_implicitly_array_convertible<typename std::iterator_traits<It>::value_type, T>::value
			&& std::is_same<typename std::iterator_traits<It>::iterator_category, std::random_access_iterator_tag>::value
		, bool> = true
	>
	constexpr void reset(It begin, It end) noexcept
	{
		m_data = &*begin;
		m_size = end - begin;
	}

	UniqueBuffer<std::remove_const_t<T>> copy() const
	{
		using U = std::remove_const_t<T>;
		UniqueBuffer<U> ret = make_buffer_for_overwrite<U>(m_size);
		for (size_t i = 0; i != m_size; ++i)
			ret[i] = m_data[i];
		return ret;
	}

private:
	T *m_data = nullptr;
	size_t m_size = 0;
};

template <typename T>
using ConstView = View<const T>;


template <typename T>
UniqueBuffer<std::remove_const_t<T>> SharedBuffer<T>::copy() const
{
	return static_cast<View<T>>(*this).copy();
}

template <typename T>
UniqueBuffer<std::remove_const_t<T>> UniqueBuffer<T>::copy() const
{
	return static_cast<View<T>>(*this).copy();
}


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

/*
Minetest
Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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
#include <type_traits>
#include "irrlichttypes.h"
#include "IReferenceCounted.h"

/** Shared pointer for IrrLicht objects.
 *
 * It should only be used for user-managed objects, i.e. those created with
 * the @c new operator or @c create* functions, like:
 * `irr_ptr<scene::IMeshBuffer> buf{new scene::SMeshBuffer()};`
 *
 * It should *never* be used for engine-managed objects, including
 * those created with @c addTexture and similar methods.
 */
template <class ReferenceCounted,
		class = typename std::enable_if<std::is_base_of<IReferenceCounted,
				ReferenceCounted>::value>::type>
class irr_ptr
{
	ReferenceCounted *value = nullptr;

	/** Drops stored pointer replacing it with the given one.
	 * @note Copy semantics: reference counter *is* increased.
	 */
	void grab(ReferenceCounted *object)
	{
		if (object)
			object->grab();
		reset(object);
	}

public:
	irr_ptr() {}

	irr_ptr(std::nullptr_t) noexcept {}

	irr_ptr(const irr_ptr &b) noexcept { grab(b.get()); }

	irr_ptr(irr_ptr &&b) noexcept { reset(b.release()); }

	template <typename B, class = typename std::enable_if<std::is_convertible<B *,
					      ReferenceCounted *>::value>::type>
	irr_ptr(const irr_ptr<B> &b) noexcept
	{
		grab(b.get());
	}

	template <typename B, class = typename std::enable_if<std::is_convertible<B *,
					      ReferenceCounted *>::value>::type>
	irr_ptr(irr_ptr<B> &&b) noexcept
	{
		reset(b.release());
	}

	/** Constructs a shared pointer out of a plain one
	 * @note Move semantics: reference counter is *not* increased.
	 */
	explicit irr_ptr(ReferenceCounted *object) noexcept { reset(object); }

	~irr_ptr() { reset(); }

	irr_ptr &operator=(const irr_ptr &b) noexcept
	{
		grab(b.get());
		return *this;
	}

	irr_ptr &operator=(irr_ptr &&b) noexcept
	{
		reset(b.release());
		return *this;
	}

	template <typename B, class = typename std::enable_if<std::is_convertible<B *,
					      ReferenceCounted *>::value>::type>
	irr_ptr &operator=(const irr_ptr<B> &b) noexcept
	{
		grab(b.get());
		return *this;
	}

	template <typename B, class = typename std::enable_if<std::is_convertible<B *,
					      ReferenceCounted *>::value>::type>
	irr_ptr &operator=(irr_ptr<B> &&b) noexcept
	{
		reset(b.release());
		return *this;
	}

	ReferenceCounted &operator*() const noexcept { return *value; }
	ReferenceCounted *operator->() const noexcept { return value; }
	explicit operator ReferenceCounted *() const noexcept { return value; }
	explicit operator bool() const noexcept { return !!value; }

	/** Returns the stored pointer.
	 */
	ReferenceCounted *get() const noexcept { return value; }

	/** Returns the stored pointer, erasing it from this class.
	 * @note Move semantics: reference counter is not changed.
	 */
	ReferenceCounted *release() noexcept
	{
		ReferenceCounted *object = value;
		value = nullptr;
		return object;
	}

	/** Drops stored pointer replacing it with the given one.
	 * @note Move semantics: reference counter is *not* increased.
	 */
	void reset(ReferenceCounted *object = nullptr) noexcept
	{
		if (value)
			value->drop();
		value = object;
	}
};

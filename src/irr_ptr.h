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

template <class ReferenceCounted, class = typename std::enable_if <
	std::is_base_of <IReferenceCounted, ReferenceCounted>::value>::type>
class irr_ptr
{
	ReferenceCounted *value = nullptr;

	void grab(ReferenceCounted *object)
	{
		if (object)
			object->grab();
		reset(object);
	}

public:
	irr_ptr()
	{
	}

	irr_ptr(std::nullptr_t) noexcept
	{
	}

	irr_ptr(const irr_ptr &b) noexcept
	{
		grab(b.get());
	}

	irr_ptr(irr_ptr &&b) noexcept
	{
		reset(b.release());
	}

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr(const irr_ptr<B> &b) noexcept
	{
		grab(b.get());
	}

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr(irr_ptr<B> &&b) noexcept
	{
		reset(b.release());
	}

	explicit irr_ptr(ReferenceCounted *object) noexcept
	{
		reset(object);
	}

	~irr_ptr()
	{
		reset();
	}

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

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr &operator=(const irr_ptr<B> &b) noexcept
	{
		grab(b.get());
		return *this;
	}

	template <typename B, class = typename std::enable_if <
		std::is_convertible <B *, ReferenceCounted *>::value>::type>
	irr_ptr &operator=(irr_ptr<B> &&b) noexcept
	{
		reset(b.release());
		return *this;
	}

	ReferenceCounted &operator*() const noexcept { return *value; }
	ReferenceCounted *operator->() const noexcept { return value; }
	explicit operator ReferenceCounted*() const noexcept { return value; }
	explicit operator bool() const noexcept { return !!value; }

	ReferenceCounted *get() const noexcept
	{
		return value;
	}

	ReferenceCounted *release() noexcept
	{
		ReferenceCounted *object = value;
		value = nullptr;
		return object;
	}

	void reset(ReferenceCounted *object = nullptr) noexcept
	{
		if (value)
			value->drop();
		value = object;
	}
};

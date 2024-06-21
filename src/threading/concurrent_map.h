/*
Copyright (C) 2024 proller <proler@gmail.com>
*/

/*
This file is part of Freeminer.

Freeminer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Freeminer  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <map>

#include "lock.h"

template <class LOCKER, class Key, class T, class Compare = std::less<Key>,
		class Allocator = std::allocator<std::pair<const Key, T>>>
class concurrent_map_ : public std::map<Key, T, Compare, Allocator>, public LOCKER
{
public:
	typedef typename std::map<Key, T, Compare, Allocator> full_type;
	typedef Key key_type;
	typedef T mapped_type;

	mapped_type &operator[](const key_type &k) = delete;
	mapped_type &operator[](key_type &&k) = delete;

	mapped_type nothing = {};

	template <typename... Args>
	mapped_type& get(Args &&...args)
	{
		auto lock = LOCKER::lock_shared_rec();

		//if (!full_type::contains(std::forward<Args>(args)...))
		if (full_type::find(std::forward<Args>(args)...) == full_type::end())
			return nothing;

		return full_type::operator[](std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) at(Args &&...args)
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::at(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) assign(Args &&...args)
	{
		auto lock = LOCKER::lock_unique_rec();
		return full_type::assign(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) insert(Args &&...args)
	{
		auto lock = LOCKER::lock_unique_rec();
		return full_type::insert(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) emplace(Args &&...args)
	{
		auto lock = LOCKER::lock_unique_rec();
		return full_type::emplace(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) emplace_try(Args &&...args)
	{
		auto lock = LOCKER::try_lock_unique_rec();
		if (!lock->owns_lock())
			return false;
		return full_type::emplace(std::forward<Args>(args)...).second;
	}

	template <typename... Args>
	decltype(auto) insert_or_assign(Args &&...args)
	{
		auto lock = LOCKER::lock_unique_rec();
		return full_type::insert_or_assign(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) empty(Args &&...args) const noexcept
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::empty(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) size(Args &&...args) const
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::size(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) count(Args &&...args) const
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::count(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) contains(Args &&...args) const
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::contains(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) find(Args &&...args)
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::find(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) begin(Args &&...args)
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::begin(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) rbegin(Args &&...args)
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::rbegin(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) end(Args &&...args)
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::end(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) rend(Args &&...args)
	{
		auto lock = LOCKER::lock_shared_rec();
		return full_type::rend(std::forward<Args>(args)...);
	}


	template <typename... Args>
	decltype(auto) erase(Args &&...args)
	{
		auto lock = LOCKER::lock_unique_rec();
		return full_type::erase(std::forward<Args>(args)...);
	}

	template <typename... Args>
	decltype(auto) clear(Args &&...args)
	{
		auto lock = LOCKER::lock_unique_rec();
		return full_type::clear(std::forward<Args>(args)...);
	}
};

template <class Key, class T, class Compare = std::less<Key>,
		class Allocator = std::allocator<std::pair<const Key, T>>>
using concurrent_map = concurrent_map_<locker<>, Key, T, Compare, Allocator>;

template <class Key, class T, class Compare = std::less<Key>, class Allocator = std::allocator<std::pair<const Key, T>>>
using concurrent_shared_map = concurrent_map_<shared_locker, Key, T, Compare, Allocator>;

#if ENABLE_THREADS

template <class Key, class T, class Compare = std::less<Key>,
		class Allocator = std::allocator<std::pair<const Key, T>>>
using maybe_concurrent_map = concurrent_map<Key, T, Compare, Allocator>;

#else

template <class Key, class T, class Compare = std::less<Key>,
		class Allocator = std::allocator<std::pair<const Key, T>>>
class not_concurrent_map : public std::map<Key, T, Compare, Allocator>,
						   public dummy_locker
{
public:
	typedef typename std::map<Key, T, Compare, Allocator> full_type;
	typedef Key key_type;
	typedef T mapped_type;

	mapped_type &get(const key_type &k) { return full_type::operator[](k); }
};

template <class Key, class T, class Compare = std::less<Key>,
		class Allocator = std::allocator<std::pair<const Key, T>>>
using maybe_concurrent_map = not_concurrent_map<Key, T, Compare, Allocator>;

#endif
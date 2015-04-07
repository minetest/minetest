/*
Minetest
Copyright (C) 2015 ShadowNinja <shadowninja@minetest.net>

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

#ifndef THREADING_ATOMIC_H
#define THREADING_ATOMIC_H


#if __cplusplus >= 201103L
	#include <atomic>
	template<typename T> using Atomic = std::atomic<T>;
#else

#define GCC_VERSION   (__GNUC__        * 100 + __GNUC_MINOR__)
#define CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#if GCC_VERSION >= 407 || CLANG_VERSION >= 302
	#define ATOMIC_LOAD(T, v)      return __atomic_load_n(&(v),       __ATOMIC_SEQ_CST)
	#define ATOMIC_STORE(T, v, x)  __atomic_store (&(v), &(x), __ATOMIC_SEQ_CST); return x
	#define ATOMIC_ADD_EQ(T, v, x) return __atomic_add_fetch(&(v), (x), __ATOMIC_SEQ_CST)
	#define ATOMIC_SUB_EQ(T, v, x) return __atomic_sub_fetch(&(v), (x), __ATOMIC_SEQ_CST)
	#define ATOMIC_POST_INC(T, v)  return __atomic_fetch_add(&(v), 1, __ATOMIC_SEQ_CST)
	#define ATOMIC_POST_DEC(T, v)  return __atomic_fetch_sub(&(v), 1, __ATOMIC_SEQ_CST)
#else
	#define ATOMIC_USE_LOCK
	#include "threading/mutex.h"

	#define ATOMIC_LOCK_OP(T, op) do { \
			mutex.lock(); \
			T _val = (op); \
			mutex.unlock(); \
			return _val; \
		} while (0)
	#define ATOMIC_LOAD(T, v) \
		if (sizeof(T) <= sizeof(void*)) return v; \
		else ATOMIC_LOCK_OP(T, v);
	#define ATOMIC_STORE(T, v, x) \
		if (sizeof(T) <= sizeof(void*)) return v = x; \
		else ATOMIC_LOCK_OP(T, v = x);
# if GCC_VERSION >= 401
	#define ATOMIC_ADD_EQ(T, v, x) return __sync_add_and_fetch(&(v), (x))
	#define ATOMIC_SUB_EQ(T, v, x) return __sync_sub_and_fetch(&(v), (x))
	#define ATOMIC_POST_INC(T, v)  return __sync_fetch_and_add(&(v), 1)
	#define ATOMIC_POST_DEC(T, v)  return __sync_fetch_and_sub(&(v), 1)
# else
	#define ATOMIC_ADD_EQ(T, v, x) ATOMIC_LOCK_OP(T, v += x)
	#define ATOMIC_SUB_EQ(T, v, x) ATOMIC_LOCK_OP(T, v -= x)
	#define ATOMIC_POST_INC(T, v)  ATOMIC_LOCK_OP(T, v++)
	#define ATOMIC_POST_DEC(T, v)  ATOMIC_LOCK_OP(T, v--)
# endif
#endif


template<typename T>
class Atomic
{
	// Like C++11 std::enable_if, but defaults to char since C++03 doesn't support SFINAE
	template<bool B, typename T_ = void> struct enable_if { typedef char type; };
	template<typename T_> struct enable_if<true, T_> { typedef T_ type; };
public:
	Atomic(const T &v=0) : val(v) {}

	operator T () { ATOMIC_LOAD(T, val); }
	T operator = (T x) { ATOMIC_STORE(T, val, x); }
	T operator += (T x) { ATOMIC_ADD_EQ(T, val, x); }
	T operator -= (T x) { ATOMIC_SUB_EQ(T, val, x); }
	T operator ++ () { return *this += 1; }
	T operator -- () { return *this -= 1; }
	T operator ++ (int) { ATOMIC_POST_INC(T, val); }
	T operator -- (int) { ATOMIC_POST_DEC(T, val); }

private:
	volatile T val;
#ifdef ATOMIC_USE_LOCK
	typename enable_if<sizeof(T) <= sizeof(void*), Mutex>::type mutex;
#endif
};

#endif  // C++11

#endif


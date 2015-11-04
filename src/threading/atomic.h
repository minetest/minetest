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
	template<typename T> using GenericAtomic = std::atomic<T>;
#else

#define GCC_VERSION   (__GNUC__        * 100 + __GNUC_MINOR__)
#define CLANG_VERSION (__clang_major__ * 100 + __clang_minor__)
#if GCC_VERSION >= 407 || CLANG_VERSION >= 302
	#define ATOMIC_LOAD_GENERIC(T, v) do {               \
		T _val;                                          \
		 __atomic_load(&(v), &(_val), __ATOMIC_SEQ_CST); \
		return _val;                                     \
	} while(0)
	#define ATOMIC_LOAD(T, v)        return __atomic_load_n    (&(v),       __ATOMIC_SEQ_CST)
	#define ATOMIC_STORE(T, v, x)           __atomic_store     (&(v), &(x), __ATOMIC_SEQ_CST); return x
	#define ATOMIC_EXCHANGE(T, v, x) return __atomic_exchange  (&(v), &(x), __ATOMIC_SEQ_CST)
	#define ATOMIC_ADD_EQ(T, v, x)   return __atomic_add_fetch (&(v), (x),  __ATOMIC_SEQ_CST)
	#define ATOMIC_SUB_EQ(T, v, x)   return __atomic_sub_fetch (&(v), (x),  __ATOMIC_SEQ_CST)
	#define ATOMIC_POST_INC(T, v)    return __atomic_fetch_add (&(v), 1,    __ATOMIC_SEQ_CST)
	#define ATOMIC_POST_DEC(T, v)    return __atomic_fetch_sub (&(v), 1,    __ATOMIC_SEQ_CST)
	#define ATOMIC_CAS(T, v, e, d)   return __atomic_compare_exchange(&(v), &(e), &(d), \
		false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#else
	#define ATOMIC_USE_LOCK
	#include "threading/mutex.h"

	#define ATOMIC_LOCK_OP(T, op) do { \
			m_mutex.lock();            \
			T _val = (op);             \
			m_mutex.unlock();          \
			return _val;               \
		} while (0)
	#define ATOMIC_LOCK_CAS(T, v, e, d) do { \
			m_mutex.lock();                  \
			bool _eq = (v == e);             \
			if (_eq)                         \
				v = d;                       \
			m_mutex.unlock();                \
			return _eq;                      \
		} while (0)
	#define ATOMIC_LOAD(T, v) ATOMIC_LOCK_OP(T, v)
	#define ATOMIC_LOAD_GENERIC(T, v) ATOMIC_LOAD(T, v)
	#define ATOMIC_STORE(T, v, x) ATOMIC_LOCK_OP(T, v = x)
	#define ATOMIC_EXCHANGE(T, v, x) do { \
			m_mutex.lock();               \
			T _val = v;                   \
			v = x;                        \
			m_mutex.unlock();             \
			return _val;                  \
		} while (0)
	#if GCC_VERSION >= 401
		#define ATOMIC_ADD_EQ(T, v, x) return __sync_add_and_fetch(&(v), (x))
		#define ATOMIC_SUB_EQ(T, v, x) return __sync_sub_and_fetch(&(v), (x))
		#define ATOMIC_POST_INC(T, v)  return __sync_fetch_and_add(&(v), 1)
		#define ATOMIC_POST_DEC(T, v)  return __sync_fetch_and_sub(&(v), 1)
		#define ATOMIC_CAS(T, v, e, d) return __sync_bool_compare_and_swap(&(v), &(e), (d))
	#else
		#define ATOMIC_ADD_EQ(T, v, x) ATOMIC_LOCK_OP(T, v += x)
		#define ATOMIC_SUB_EQ(T, v, x) ATOMIC_LOCK_OP(T, v -= x)
		#define ATOMIC_POST_INC(T, v)  ATOMIC_LOCK_OP(T, v++)
		#define ATOMIC_POST_DEC(T, v)  ATOMIC_LOCK_OP(T, v--)
		#define ATOMIC_CAS(T, v, e, d) ATOMIC_LOCK_CAS(T, v, e, d)
	#endif
#endif

// For usage with integral types.
template<typename T>
class Atomic {
public:
	Atomic(const T &v = 0) : m_val(v) {}

	operator T () { ATOMIC_LOAD(T, m_val); }

	T exchange(T x)  { ATOMIC_EXCHANGE(T, m_val, x); }
	bool compare_exchange_strong(T &expected, T desired) { ATOMIC_CAS(T, m_val, expected, desired); }

	T operator =  (T x) { ATOMIC_STORE(T, m_val, x); }
	T operator += (T x) { ATOMIC_ADD_EQ(T, m_val, x); }
	T operator -= (T x) { ATOMIC_SUB_EQ(T, m_val, x); }
	T operator ++ ()    { return *this += 1; }
	T operator -- ()    { return *this -= 1; }
	T operator ++ (int) { ATOMIC_POST_INC(T, m_val); }
	T operator -- (int) { ATOMIC_POST_DEC(T, m_val); }
private:
	T m_val;
#ifdef ATOMIC_USE_LOCK
	Mutex m_mutex;
#endif
};

// For usage with non-integral types like float for example.
// Needed because the other operations aren't provided by gcc
// for non-integral types:
// https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/_005f_005fatomic-Builtins.html
template<typename T>
class GenericAtomic {
public:
	GenericAtomic(const T &v = 0) : m_val(v) {}

	operator T () { ATOMIC_LOAD_GENERIC(T, m_val); }

	T exchange(T x)  { ATOMIC_EXCHANGE(T, m_val, x); }
	bool compare_exchange_strong(T &expected, T desired) { ATOMIC_CAS(T, m_val, expected, desired); }

	T operator = (T x) { ATOMIC_STORE(T, m_val, x); }
private:
	T m_val;
#ifdef ATOMIC_USE_LOCK
	Mutex m_mutex;
#endif
};

#endif  // C++11

#endif

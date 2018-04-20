/*
   Basic Unicode string class for Irrlicht.
   Copyright (c) 2009-2011 John Norman

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this class can be located at:
   http://irrlicht.suckerfreegames.com/

   John Norman
   john@suckerfreegames.com
*/

#ifndef __IRR_USTRING_H_INCLUDED__
#define __IRR_USTRING_H_INCLUDED__

#if (__cplusplus > 199711L) || (defined(_MSC_VER) && _MSC_VER >= 1600) || defined(__GXX_EXPERIMENTAL_CXX0X__)
#	define USTRING_CPP0X
#	if defined(__GXX_EXPERIMENTAL_CXX0X__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5)))
#		define USTRING_CPP0X_NEWLITERALS
#	endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#define __BYTE_ORDER 0
#define __LITTLE_ENDIAN 0
#define __BIG_ENDIAN 1
#elif defined(__MACH__) && defined(__APPLE__)
#include <machine/endian.h>
#elif defined(__FreeBSD__)
#include <sys/endian.h>
#else
#include <endian.h>
#endif

#ifdef USTRING_CPP0X
#	include <utility>
#endif

#ifndef USTRING_NO_STL
#	include <string>
#	include <iterator>
#	include <ostream>
#endif

#include "irrTypes.h"
#include "irrAllocator.h"
#include "irrArray.h"
#include "irrMath.h"
#include "irrString.h"
#include "path.h"

//! UTF-16 surrogate start values.
static const irr::u16 UTF16_HI_SURROGATE = 0xD800;
static const irr::u16 UTF16_LO_SURROGATE = 0xDC00;

//! Is a UTF-16 code point a surrogate?
#define UTF16_IS_SURROGATE(c)		(((c) & 0xF800) == 0xD800)
#define UTF16_IS_SURROGATE_HI(c)	(((c) & 0xFC00) == 0xD800)
#define UTF16_IS_SURROGATE_LO(c)	(((c) & 0xFC00) == 0xDC00)


namespace irr
{

	// Define our character types.
#ifdef USTRING_CPP0X_NEWLITERALS	// C++0x
	typedef char32_t uchar32_t;
	typedef char16_t uchar16_t;
	typedef char uchar8_t;
#else
	typedef u32 uchar32_t;
	typedef u16 uchar16_t;
	typedef u8 uchar8_t;
#endif

namespace core
{

namespace unicode
{

//! The unicode replacement character.  Used to replace invalid characters.
const irr::u16 UTF_REPLACEMENT_CHARACTER = 0xFFFD;

//! Convert a UTF-16 surrogate pair into a UTF-32 character.
//! \param high The high value of the pair.
//! \param low The low value of the pair.
//! \return The UTF-32 character expressed by the surrogate pair.
inline uchar32_t toUTF32(uchar16_t high, uchar16_t low)
{
	// Convert the surrogate pair into a single UTF-32 character.
	uchar32_t x = ((high & ((1 << 6) -1)) << 10) | (low & ((1 << 10) -1));
	uchar32_t wu = ((high >> 6) & ((1 << 5) - 1)) + 1;
	return (wu << 16) | x;
}

//! Swaps the endianness of a 16-bit value.
//! \return The new value.
inline uchar16_t swapEndian16(const uchar16_t& c)
{
	return ((c >> 8) & 0x00FF) | ((c << 8) & 0xFF00);
}

//! Swaps the endianness of a 32-bit value.
//! \return The new value.
inline uchar32_t swapEndian32(const uchar32_t& c)
{
	return  ((c >> 24) & 0x000000FF) |
			((c >> 8)  & 0x0000FF00) |
			((c << 8)  & 0x00FF0000) |
			((c << 24) & 0xFF000000);
}

//! The Unicode byte order mark.
const u16 BOM = 0xFEFF;

//! The size of the Unicode byte order mark in terms of the Unicode character size.
const u8 BOM_UTF8_LEN = 3;
const u8 BOM_UTF16_LEN = 1;
const u8 BOM_UTF32_LEN = 1;

//! Unicode byte order marks for file operations.
const u8 BOM_ENCODE_UTF8[3] = { 0xEF, 0xBB, 0xBF };
const u8 BOM_ENCODE_UTF16_BE[2] = { 0xFE, 0xFF };
const u8 BOM_ENCODE_UTF16_LE[2] = { 0xFF, 0xFE };
const u8 BOM_ENCODE_UTF32_BE[4] = { 0x00, 0x00, 0xFE, 0xFF };
const u8 BOM_ENCODE_UTF32_LE[4] = { 0xFF, 0xFE, 0x00, 0x00 };

//! The size in bytes of the Unicode byte marks for file operations.
const u8 BOM_ENCODE_UTF8_LEN = 3;
const u8 BOM_ENCODE_UTF16_LEN = 2;
const u8 BOM_ENCODE_UTF32_LEN = 4;

//! Unicode encoding type.
enum EUTF_ENCODE
{
	EUTFE_NONE		= 0,
	EUTFE_UTF8,
	EUTFE_UTF16,
	EUTFE_UTF16_LE,
	EUTFE_UTF16_BE,
	EUTFE_UTF32,
	EUTFE_UTF32_LE,
	EUTFE_UTF32_BE
};

//! Unicode endianness.
enum EUTF_ENDIAN
{
	EUTFEE_NATIVE	= 0,
	EUTFEE_LITTLE,
	EUTFEE_BIG
};

//! Returns the specified unicode byte order mark in a byte array.
//! The byte order mark is the first few bytes in a text file that signifies its encoding.
/** \param mode The Unicode encoding method that we want to get the byte order mark for.
		If EUTFE_UTF16 or EUTFE_UTF32 is passed, it uses the native system endianness. **/
//! \return An array that contains a byte order mark.
inline core::array<u8> getUnicodeBOM(EUTF_ENCODE mode)
{
#define COPY_ARRAY(source, size) \
	memcpy(ret.pointer(), source, size); \
	ret.set_used(size)

	core::array<u8> ret(4);
	switch (mode)
	{
		case EUTFE_UTF8:
			COPY_ARRAY(BOM_ENCODE_UTF8, BOM_ENCODE_UTF8_LEN);
			break;
		case EUTFE_UTF16:
			#ifdef __BIG_ENDIAN__
				COPY_ARRAY(BOM_ENCODE_UTF16_BE, BOM_ENCODE_UTF16_LEN);
			#else
				COPY_ARRAY(BOM_ENCODE_UTF16_LE, BOM_ENCODE_UTF16_LEN);
			#endif
			break;
		case EUTFE_UTF16_BE:
			COPY_ARRAY(BOM_ENCODE_UTF16_BE, BOM_ENCODE_UTF16_LEN);
			break;
		case EUTFE_UTF16_LE:
			COPY_ARRAY(BOM_ENCODE_UTF16_LE, BOM_ENCODE_UTF16_LEN);
			break;
		case EUTFE_UTF32:
			#ifdef __BIG_ENDIAN__
				COPY_ARRAY(BOM_ENCODE_UTF32_BE, BOM_ENCODE_UTF32_LEN);
			#else
				COPY_ARRAY(BOM_ENCODE_UTF32_LE, BOM_ENCODE_UTF32_LEN);
			#endif
			break;
		case EUTFE_UTF32_BE:
			COPY_ARRAY(BOM_ENCODE_UTF32_BE, BOM_ENCODE_UTF32_LEN);
			break;
		case EUTFE_UTF32_LE:
			COPY_ARRAY(BOM_ENCODE_UTF32_LE, BOM_ENCODE_UTF32_LEN);
			break;
		case EUTFE_NONE:
			// TODO sapier: fixed warning only,
			// don't know if something needs to be done here
			break;
	}
	return ret;

#undef COPY_ARRAY
}

//! Detects if the given data stream starts with a unicode BOM.
//! \param data The data stream to check.
//! \return The unicode BOM associated with the data stream, or EUTFE_NONE if none was found.
inline EUTF_ENCODE determineUnicodeBOM(const char* data)
{
	if (memcmp(data, BOM_ENCODE_UTF8, 3) == 0) return EUTFE_UTF8;
	if (memcmp(data, BOM_ENCODE_UTF16_BE, 2) == 0) return EUTFE_UTF16_BE;
	if (memcmp(data, BOM_ENCODE_UTF16_LE, 2) == 0) return EUTFE_UTF16_LE;
	if (memcmp(data, BOM_ENCODE_UTF32_BE, 4) == 0) return EUTFE_UTF32_BE;
	if (memcmp(data, BOM_ENCODE_UTF32_LE, 4) == 0) return EUTFE_UTF32_LE;
	return EUTFE_NONE;
}

} // end namespace unicode


//! UTF-16 string class.
template <typename TAlloc = irrAllocator<uchar16_t> >
class ustring16
{
public:

	///------------------///
	/// iterator classes ///
	///------------------///

	//! Access an element in a unicode string, allowing one to change it.
	class _ustring16_iterator_access
	{
		public:
			_ustring16_iterator_access(const ustring16<TAlloc>* s, u32 p) : ref(s), pos(p) {}

			//! Allow the class to be interpreted as a single UTF-32 character.
			operator uchar32_t() const
			{
				return _get();
			}

			//! Allow one to change the character in the unicode string.
			//! \param c The new character to use.
			//! \return Myself.
			_ustring16_iterator_access& operator=(const uchar32_t c)
			{
				_set(c);
				return *this;
			}

			//! Increments the value by 1.
			//! \return Myself.
			_ustring16_iterator_access& operator++()
			{
				_set(_get() + 1);
				return *this;
			}

			//! Increments the value by 1, returning the old value.
			//! \return A unicode character.
			uchar32_t operator++(int)
			{
				uchar32_t old = _get();
				_set(old + 1);
				return old;
			}

			//! Decrements the value by 1.
			//! \return Myself.
			_ustring16_iterator_access& operator--()
			{
				_set(_get() - 1);
				return *this;
			}

			//! Decrements the value by 1, returning the old value.
			//! \return A unicode character.
			uchar32_t operator--(int)
			{
				uchar32_t old = _get();
				_set(old - 1);
				return old;
			}

			//! Adds to the value by a specified amount.
			//! \param val The amount to add to this character.
			//! \return Myself.
			_ustring16_iterator_access& operator+=(int val)
			{
				_set(_get() + val);
				return *this;
			}

			//! Subtracts from the value by a specified amount.
			//! \param val The amount to subtract from this character.
			//! \return Myself.
			_ustring16_iterator_access& operator-=(int val)
			{
				_set(_get() - val);
				return *this;
			}

			//! Multiples the value by a specified amount.
			//! \param val The amount to multiply this character by.
			//! \return Myself.
			_ustring16_iterator_access& operator*=(int val)
			{
				_set(_get() * val);
				return *this;
			}

			//! Divides the value by a specified amount.
			//! \param val The amount to divide this character by.
			//! \return Myself.
			_ustring16_iterator_access& operator/=(int val)
			{
				_set(_get() / val);
				return *this;
			}

			//! Modulos the value by a specified amount.
			//! \param val The amount to modulo this character by.
			//! \return Myself.
			_ustring16_iterator_access& operator%=(int val)
			{
				_set(_get() % val);
				return *this;
			}

			//! Adds to the value by a specified amount.
			//! \param val The amount to add to this character.
			//! \return A unicode character.
			uchar32_t operator+(int val) const
			{
				return _get() + val;
			}

			//! Subtracts from the value by a specified amount.
			//! \param val The amount to subtract from this character.
			//! \return A unicode character.
			uchar32_t operator-(int val) const
			{
				return _get() - val;
			}

			//! Multiplies the value by a specified amount.
			//! \param val The amount to multiply this character by.
			//! \return A unicode character.
			uchar32_t operator*(int val) const
			{
				return _get() * val;
			}

			//! Divides the value by a specified amount.
			//! \param val The amount to divide this character by.
			//! \return A unicode character.
			uchar32_t operator/(int val) const
			{
				return _get() / val;
			}

			//! Modulos the value by a specified amount.
			//! \param val The amount to modulo this character by.
			//! \return A unicode character.
			uchar32_t operator%(int val) const
			{
				return _get() % val;
			}

		private:
			//! Gets a uchar32_t from our current position.
			uchar32_t _get() const
			{
				const uchar16_t* a = ref->c_str();
				if (!UTF16_IS_SURROGATE(a[pos]))
					return static_cast<uchar32_t>(a[pos]);
				else
				{
					if (pos + 1 >= ref->size_raw())
						return 0;

					return unicode::toUTF32(a[pos], a[pos + 1]);
				}
			}

			//! Sets a uchar32_t at our current position.
			void _set(uchar32_t c)
			{
				ustring16<TAlloc>* ref2 = const_cast<ustring16<TAlloc>*>(ref);
				const uchar16_t* a = ref2->c_str();
				if (c > 0xFFFF)
				{
					// c will be multibyte, so split it up into the high and low surrogate pairs.
					uchar16_t x = static_cast<uchar16_t>(c);
					uchar16_t vh = UTF16_HI_SURROGATE | ((((c >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
					uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));

					// If the previous position was a surrogate pair, just replace them.  Else, insert the low pair.
					if (UTF16_IS_SURROGATE_HI(a[pos]) && pos + 1 != ref2->size_raw())
						ref2->replace_raw(vl, static_cast<u32>(pos) + 1);
					else ref2->insert_raw(vl, static_cast<u32>(pos) + 1);

					ref2->replace_raw(vh, static_cast<u32>(pos));
				}
				else
				{
					// c will be a single byte.
					uchar16_t vh = static_cast<uchar16_t>(c);

					// If the previous position was a surrogate pair, remove the extra byte.
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						ref2->erase_raw(static_cast<u32>(pos) + 1);

					ref2->replace_raw(vh, static_cast<u32>(pos));
				}
			}

			const ustring16<TAlloc>* ref;
			u32 pos;
	};
	typedef typename ustring16<TAlloc>::_ustring16_iterator_access access;


	//! Iterator to iterate through a UTF-16 string.
#ifndef USTRING_NO_STL
	class _ustring16_const_iterator : public std::iterator<
		std::bidirectional_iterator_tag,	// iterator_category
		access,								// value_type
		ptrdiff_t,							// difference_type
		const access,						// pointer
		const access						// reference
	>
#else
	class _ustring16_const_iterator
#endif
	{
		public:
			typedef _ustring16_const_iterator _Iter;
			typedef std::iterator<std::bidirectional_iterator_tag, access, ptrdiff_t, const access, const access> _Base;
			typedef const access const_pointer;
			typedef const access const_reference;

#ifndef USTRING_NO_STL
			typedef typename _Base::value_type value_type;
			typedef typename _Base::difference_type difference_type;
			typedef typename _Base::difference_type distance_type;
			typedef typename _Base::pointer pointer;
			typedef const_reference reference;
#else
			typedef access value_type;
			typedef u32 difference_type;
			typedef u32 distance_type;
			typedef const_pointer pointer;
			typedef const_reference reference;
#endif

			//! Constructors.
			_ustring16_const_iterator(const _Iter& i) : ref(i.ref), pos(i.pos) {}
			_ustring16_const_iterator(const ustring16<TAlloc>& s) : ref(&s), pos(0) {}
			_ustring16_const_iterator(const ustring16<TAlloc>& s, const u32 p) : ref(&s), pos(0)
			{
				if (ref->size_raw() == 0 || p == 0)
					return;

				// Go to the appropriate position.
				u32 i = p;
				u32 sr = ref->size_raw();
				const uchar16_t* a = ref->c_str();
				while (i != 0 && pos < sr)
				{
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						pos += 2;
					else ++pos;
					--i;
				}
			}

			//! Test for equalness.
			bool operator==(const _Iter& iter) const
			{
				if (ref == iter.ref && pos == iter.pos)
					return true;
				return false;
			}

			//! Test for unequalness.
			bool operator!=(const _Iter& iter) const
			{
				if (ref != iter.ref || pos != iter.pos)
					return true;
				return false;
			}

			//! Switch to the next full character in the string.
			_Iter& operator++()
			{	// ++iterator
				if (pos == ref->size_raw()) return *this;
				const uchar16_t* a = ref->c_str();
				if (UTF16_IS_SURROGATE_HI(a[pos]))
					pos += 2;			// TODO: check for valid low surrogate?
				else ++pos;
				if (pos > ref->size_raw()) pos = ref->size_raw();
				return *this;
			}

			//! Switch to the next full character in the string, returning the previous position.
			_Iter operator++(int)
			{	// iterator++
				_Iter _tmp(*this);
				++*this;
				return _tmp;
			}

			//! Switch to the previous full character in the string.
			_Iter& operator--()
			{	// --iterator
				if (pos == 0) return *this;
				const uchar16_t* a = ref->c_str();
				--pos;
				if (UTF16_IS_SURROGATE_LO(a[pos]) && pos != 0)	// low surrogate, go back one more.
					--pos;
				return *this;
			}

			//! Switch to the previous full character in the string, returning the previous position.
			_Iter operator--(int)
			{	// iterator--
				_Iter _tmp(*this);
				--*this;
				return _tmp;
			}

			//! Advance a specified number of full characters in the string.
			//! \return Myself.
			_Iter& operator+=(const difference_type v)
			{
				if (v == 0) return *this;
				if (v < 0) return operator-=(v * -1);

				if (pos >= ref->size_raw())
					return *this;

				// Go to the appropriate position.
				// TODO: Don't force u32 on an x64 OS.  Make it agnostic.
				u32 i = (u32)v;
				u32 sr = ref->size_raw();
				const uchar16_t* a = ref->c_str();
				while (i != 0 && pos < sr)
				{
					if (UTF16_IS_SURROGATE_HI(a[pos]))
						pos += 2;
					else ++pos;
					--i;
				}
				if (pos > sr)
					pos = sr;

				return *this;
			}

			//! Go back a specified number of full characters in the string.
			//! \return Myself.
			_Iter& operator-=(const difference_type v)
			{
				if (v == 0) return *this;
				if (v > 0) return operator+=(v * -1);

				if (pos == 0)
					return *this;

				// Go to the appropriate position.
				// TODO: Don't force u32 on an x64 OS.  Make it agnostic.
				u32 i = (u32)v;
				const uchar16_t* a = ref->c_str();
				while (i != 0 && pos != 0)
				{
					--pos;
					if (UTF16_IS_SURROGATE_LO(a[pos]) != 0 && pos != 0)
						--pos;
					--i;
				}

				return *this;
			}

			//! Return a new iterator that is a variable number of full characters forward from the current position.
			_Iter operator+(const difference_type v) const
			{
				_Iter ret(*this);
				ret += v;
				return ret;
			}

			//! Return a new iterator that is a variable number of full characters backward from the current position.
			_Iter operator-(const difference_type v) const
			{
				_Iter ret(*this);
				ret -= v;
				return ret;
			}

			//! Returns the distance between two iterators.
			difference_type operator-(const _Iter& iter) const
			{
				// Make sure we reference the same object!
				if (ref != iter.ref)
					return difference_type();

				_Iter i = iter;
				difference_type ret;

				// Walk up.
				if (pos > i.pos)
				{
					while (pos > i.pos)
					{
						++i;
						++ret;
					}
					return ret;
				}

				// Walk down.
				while (pos < i.pos)
				{
					--i;
					--ret;
				}
				return ret;
			}

			//! Accesses the full character at the iterator's position.
			const_reference operator*() const
			{
				if (pos >= ref->size_raw())
				{
					const uchar16_t* a = ref->c_str();
					u32 p = ref->size_raw();
					if (UTF16_IS_SURROGATE_LO(a[p]))
						--p;
					reference ret(ref, p);
					return ret;
				}
				const_reference ret(ref, pos);
				return ret;
			}

			//! Accesses the full character at the iterator's position.
			reference operator*()
			{
				if (pos >= ref->size_raw())
				{
					const uchar16_t* a = ref->c_str();
					u32 p = ref->size_raw();
					if (UTF16_IS_SURROGATE_LO(a[p]))
						--p;
					reference ret(ref, p);
					return ret;
				}
				reference ret(ref, pos);
				return ret;
			}

			//! Accesses the full character at the iterator's position.
			const_pointer operator->() const
			{
				return operator*();
			}

			//! Accesses the full character at the iterator's position.
			pointer operator->()
			{
				return operator*();
			}

			//! Is the iterator at the start of the string?
			bool atStart() const
			{
				return pos == 0;
			}

			//! Is the iterator at the end of the string?
			bool atEnd() const
			{
				const uchar16_t* a = ref->c_str();
				if (UTF16_IS_SURROGATE(a[pos]))
					return (pos + 1) >= ref->size_raw();
				else return pos >= ref->size_raw();
			}

			//! Moves the iterator to the start of the string.
			void toStart()
			{
				pos = 0;
			}

			//! Moves the iterator to the end of the string.
			void toEnd()
			{
				pos = ref->size_raw();
			}

			//! Returns the iterator's position.
			//! \return The iterator's position.
			u32 getPos() const
			{
				return pos;
			}

		protected:
			const ustring16<TAlloc>* ref;
			u32 pos;
	};

	//! Iterator to iterate through a UTF-16 string.
	class _ustring16_iterator : public _ustring16_const_iterator
	{
		public:
			typedef _ustring16_iterator _Iter;
			typedef _ustring16_const_iterator _Base;
			typedef typename _Base::const_pointer const_pointer;
			typedef typename _Base::const_reference const_reference;


			typedef typename _Base::value_type value_type;
			typedef typename _Base::difference_type difference_type;
			typedef typename _Base::distance_type distance_type;
			typedef access pointer;
			typedef access reference;

			using _Base::pos;
			using _Base::ref;

			//! Constructors.
			_ustring16_iterator(const _Iter& i) : _ustring16_const_iterator(i) {}
			_ustring16_iterator(const ustring16<TAlloc>& s) : _ustring16_const_iterator(s) {}
			_ustring16_iterator(const ustring16<TAlloc>& s, const u32 p) : _ustring16_const_iterator(s, p) {}

			//! Accesses the full character at the iterator's position.
			reference operator*() const
			{
				if (pos >= ref->size_raw())
				{
					const uchar16_t* a = ref->c_str();
					u32 p = ref->size_raw();
					if (UTF16_IS_SURROGATE_LO(a[p]))
						--p;
					reference ret(ref, p);
					return ret;
				}
				reference ret(ref, pos);
				return ret;
			}

			//! Accesses the full character at the iterator's position.
			reference operator*()
			{
				if (pos >= ref->size_raw())
				{
					const uchar16_t* a = ref->c_str();
					u32 p = ref->size_raw();
					if (UTF16_IS_SURROGATE_LO(a[p]))
						--p;
					reference ret(ref, p);
					return ret;
				}
				reference ret(ref, pos);
				return ret;
			}

			//! Accesses the full character at the iterator's position.
			pointer operator->() const
			{
				return operator*();
			}

			//! Accesses the full character at the iterator's position.
			pointer operator->()
			{
				return operator*();
			}
	};

	typedef typename ustring16<TAlloc>::_ustring16_iterator iterator;
	typedef typename ustring16<TAlloc>::_ustring16_const_iterator const_iterator;

	///----------------------///
	/// end iterator classes ///
	///----------------------///

	//! Default constructor
	ustring16()
	: array(0), allocated(1), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		array = allocator.allocate(1); // new u16[1];
		array[0] = 0x0;
	}


	//! Constructor
	ustring16(const ustring16<TAlloc>& other)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		*this = other;
	}


	//! Constructor from other string types
	template <class B, class A>
	ustring16(const string<B, A>& other)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		*this = other;
	}


#ifndef USTRING_NO_STL
	//! Constructor from std::string
	template <class B, class A, typename Alloc>
	ustring16(const std::basic_string<B, A, Alloc>& other)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		*this = other.c_str();
	}


	//! Constructor from iterator.
	template <typename Itr>
	ustring16(Itr first, Itr last)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif
		reserve(std::distance(first, last));
		array[used] = 0;

		for (; first != last; ++first)
			append((uchar32_t)*first);
	}
#endif


#ifndef USTRING_CPP0X_NEWLITERALS
	//! Constructor for copying a character string from a pointer.
	ustring16(const char* const c)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		loadDataStream(c, strlen(c));
		//append((uchar8_t*)c);
	}


	//! Constructor for copying a character string from a pointer with a given length.
	ustring16(const char* const c, u32 length)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		loadDataStream(c, length);
	}
#endif


	//! Constructor for copying a UTF-8 string from a pointer.
	ustring16(const uchar8_t* const c)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		append(c);
	}


	//! Constructor for copying a UTF-8 string from a single char.
	ustring16(const char c)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		append((uchar32_t)c);
	}


	//! Constructor for copying a UTF-8 string from a pointer with a given length.
	ustring16(const uchar8_t* const c, u32 length)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		append(c, length);
	}


	//! Constructor for copying a UTF-16 string from a pointer.
	ustring16(const uchar16_t* const c)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		append(c);
	}


	//! Constructor for copying a UTF-16 string from a pointer with a given length
	ustring16(const uchar16_t* const c, u32 length)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		append(c, length);
	}


	//! Constructor for copying a UTF-32 string from a pointer.
	ustring16(const uchar32_t* const c)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		append(c);
	}


	//! Constructor for copying a UTF-32 from a pointer with a given length.
	ustring16(const uchar32_t* const c, u32 length)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		append(c, length);
	}


	//! Constructor for copying a wchar_t string from a pointer.
	ustring16(const wchar_t* const c)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		if (sizeof(wchar_t) == 4)
			append(reinterpret_cast<const uchar32_t* const>(c));
		else if (sizeof(wchar_t) == 2)
			append(reinterpret_cast<const uchar16_t* const>(c));
		else if (sizeof(wchar_t) == 1)
			append(reinterpret_cast<const uchar8_t* const>(c));
	}


	//! Constructor for copying a wchar_t string from a pointer with a given length.
	ustring16(const wchar_t* const c, u32 length)
	: array(0), allocated(0), used(0)
	{
#if __BYTE_ORDER == __BIG_ENDIAN
		encoding = unicode::EUTFE_UTF16_BE;
#else
		encoding = unicode::EUTFE_UTF16_LE;
#endif

		if (sizeof(wchar_t) == 4)
			append(reinterpret_cast<const uchar32_t* const>(c), length);
		else if (sizeof(wchar_t) == 2)
			append(reinterpret_cast<const uchar16_t* const>(c), length);
		else if (sizeof(wchar_t) == 1)
			append(reinterpret_cast<const uchar8_t* const>(c), length);
	}


#ifdef USTRING_CPP0X
	//! Constructor for moving a ustring16
	ustring16(ustring16<TAlloc>&& other)
	: array(other.array), encoding(other.encoding), allocated(other.allocated), used(other.used)
	{
		//std::cout << "MOVE constructor" << std::endl;
		other.array = 0;
		other.allocated = 0;
		other.used = 0;
	}
#endif


	//! Destructor
	~ustring16()
	{
		allocator.deallocate(array); // delete [] array;
	}


	//! Assignment operator
	ustring16& operator=(const ustring16<TAlloc>& other)
	{
		if (this == &other)
			return *this;

		used = other.size_raw();
		if (used >= allocated)
		{
			allocator.deallocate(array); // delete [] array;
			allocated = used + 1;
			array = allocator.allocate(used + 1); //new u16[used];
		}

		const uchar16_t* p = other.c_str();
		for (u32 i=0; i<=used; ++i, ++p)
			array[i] = *p;

		array[used] = 0;

		// Validate our new UTF-16 string.
		validate();

		return *this;
	}


#ifdef USTRING_CPP0X
	//! Move assignment operator
	ustring16& operator=(ustring16<TAlloc>&& other)
	{
		if (this != &other)
		{
			//std::cout << "MOVE operator=" << std::endl;
			allocator.deallocate(array);

			array = other.array;
			allocated = other.allocated;
			encoding = other.encoding;
			used = other.used;
			other.array = 0;
			other.used = 0;
		}
		return *this;
	}
#endif


	//! Assignment operator for other string types
	template <class B, class A>
	ustring16<TAlloc>& operator=(const string<B, A>& other)
	{
		*this = other.c_str();
		return *this;
	}


	//! Assignment operator for UTF-8 strings
	ustring16<TAlloc>& operator=(const uchar8_t* const c)
	{
		if (!array)
		{
			array = allocator.allocate(1); //new u16[1];
			allocated = 1;
		}
		used = 0;
		array[used] = 0x0;
		if (!c) return *this;

		//! Append our string now.
		append(c);
		return *this;
	}


	//! Assignment operator for UTF-16 strings
	ustring16<TAlloc>& operator=(const uchar16_t* const c)
	{
		if (!array)
		{
			array = allocator.allocate(1); //new u16[1];
			allocated = 1;
		}
		used = 0;
		array[used] = 0x0;
		if (!c) return *this;

		//! Append our string now.
		append(c);
		return *this;
	}


	//! Assignment operator for UTF-32 strings
	ustring16<TAlloc>& operator=(const uchar32_t* const c)
	{
		if (!array)
		{
			array = allocator.allocate(1); //new u16[1];
			allocated = 1;
		}
		used = 0;
		array[used] = 0x0;
		if (!c) return *this;

		//! Append our string now.
		append(c);
		return *this;
	}


	//! Assignment operator for wchar_t strings.
	/** Note that this assumes that a correct unicode string is stored in the wchar_t string.
		Since wchar_t changes depending on its platform, it could either be a UTF-8, -16, or -32 string.
		This function assumes you are storing the correct unicode encoding inside the wchar_t string. **/
	ustring16<TAlloc>& operator=(const wchar_t* const c)
	{
		if (sizeof(wchar_t) == 4)
			*this = reinterpret_cast<const uchar32_t* const>(c);
		else if (sizeof(wchar_t) == 2)
			*this = reinterpret_cast<const uchar16_t* const>(c);
		else if (sizeof(wchar_t) == 1)
			*this = reinterpret_cast<const uchar8_t* const>(c);

		return *this;
	}


	//! Assignment operator for other strings.
	/** Note that this assumes that a correct unicode string is stored in the string. **/
	template <class B>
	ustring16<TAlloc>& operator=(const B* const c)
	{
		if (sizeof(B) == 4)
			*this = reinterpret_cast<const uchar32_t* const>(c);
		else if (sizeof(B) == 2)
			*this = reinterpret_cast<const uchar16_t* const>(c);
		else if (sizeof(B) == 1)
			*this = reinterpret_cast<const uchar8_t* const>(c);

		return *this;
	}


	//! Direct access operator
	access operator [](const u32 index)
	{
		_IRR_DEBUG_BREAK_IF(index>=size()) // bad index
		iterator iter(*this, index);
		return iter.operator*();
	}


	//! Direct access operator
	const access operator [](const u32 index) const
	{
		_IRR_DEBUG_BREAK_IF(index>=size()) // bad index
		const_iterator iter(*this, index);
		return iter.operator*();
	}


	//! Equality operator
	bool operator ==(const uchar16_t* const str) const
	{
		if (!str)
			return false;

		u32 i;
		for(i=0; array[i] && str[i]; ++i)
			if (array[i] != str[i])
				return false;

		return !array[i] && !str[i];
	}


	//! Equality operator
	bool operator ==(const ustring16<TAlloc>& other) const
	{
		for(u32 i=0; array[i] && other.array[i]; ++i)
			if (array[i] != other.array[i])
				return false;

		return used == other.used;
	}


	//! Is smaller comparator
	bool operator <(const ustring16<TAlloc>& other) const
	{
		for(u32 i=0; array[i] && other.array[i]; ++i)
		{
			s32 diff = array[i] - other.array[i];
			if ( diff )
				return diff < 0;
		}

		return used < other.used;
	}


	//! Inequality operator
	bool operator !=(const uchar16_t* const str) const
	{
		return !(*this == str);
	}


	//! Inequality operator
	bool operator !=(const ustring16<TAlloc>& other) const
	{
		return !(*this == other);
	}


	//! Returns the length of a ustring16 in full characters.
	//! \return Length of a ustring16 in full characters.
	u32 size() const
	{
		const_iterator i(*this, 0);
		u32 pos = 0;
		while (!i.atEnd())
		{
			++i;
			++pos;
		}
		return pos;
	}


	//! Informs if the ustring is empty or not.
	//! \return True if the ustring is empty, false if not.
	bool empty() const
	{
		return (size_raw() == 0);
	}


	//! Returns a pointer to the raw UTF-16 string data.
	//! \return pointer to C-style NUL terminated array of UTF-16 code points.
	const uchar16_t* c_str() const
	{
		return array;
	}


	//! Compares the first n characters of this string with another.
	//! \param other Other string to compare to.
	//! \param n Number of characters to compare.
	//! \return True if the n first characters of both strings are equal.
	bool equalsn(const ustring16<TAlloc>& other, u32 n) const
	{
		u32 i;
		const uchar16_t* oa = other.c_str();
		for(i=0; array[i] && oa[i] && i < n; ++i)
			if (array[i] != oa[i])
				return false;

		// if one (or both) of the strings was smaller then they
		// are only equal if they have the same length
		return (i == n) || (used == other.used);
	}


	//! Compares the first n characters of this string with another.
	//! \param str Other string to compare to.
	//! \param n Number of characters to compare.
	//! \return True if the n first characters of both strings are equal.
	bool equalsn(const uchar16_t* const str, u32 n) const
	{
		if (!str)
			return false;
		u32 i;
		for(i=0; array[i] && str[i] && i < n; ++i)
			if (array[i] != str[i])
				return false;

		// if one (or both) of the strings was smaller then they
		// are only equal if they have the same length
		return (i == n) || (array[i] == 0 && str[i] == 0);
	}


	//! Appends a character to this ustring16
	//! \param character The character to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& append(uchar32_t character)
	{
		if (used + 2 >= allocated)
			reallocate(used + 2);

		if (character > 0xFFFF)
		{
			used += 2;

			// character will be multibyte, so split it up into a surrogate pair.
			uchar16_t x = static_cast<uchar16_t>(character);
			uchar16_t vh = UTF16_HI_SURROGATE | ((((character >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
			uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));
			array[used-2] = vh;
			array[used-1] = vl;
		}
		else
		{
			++used;
			array[used-1] = character;
		}
		array[used] = 0;

		return *this;
	}


	//! Appends a UTF-8 string to this ustring16
	//! \param other The UTF-8 string to append.
	//! \param length The length of the string to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& append(const uchar8_t* const other, u32 length=0xffffffff)
	{
		if (!other)
			return *this;

		// Determine if the string is long enough for a BOM.
		u32 len = 0;
		const uchar8_t* p = other;
		do
		{
			++len;
		} while (*p++ && len < unicode::BOM_ENCODE_UTF8_LEN);

		// Check for BOM.
		unicode::EUTF_ENCODE c_bom = unicode::EUTFE_NONE;
		if (len == unicode::BOM_ENCODE_UTF8_LEN)
		{
			if (memcmp(other, unicode::BOM_ENCODE_UTF8, unicode::BOM_ENCODE_UTF8_LEN) == 0)
				c_bom = unicode::EUTFE_UTF8;
		}

		// If a BOM was found, don't include it in the string.
		const uchar8_t* c2 = other;
		if (c_bom != unicode::EUTFE_NONE)
		{
			c2 = other + unicode::BOM_UTF8_LEN;
			length -= unicode::BOM_UTF8_LEN;
		}

		// Calculate the size of the string to read in.
		len = 0;
		p = c2;
		do
		{
			++len;
		} while(*p++ && len < length);
		if (len > length)
			len = length;

		// If we need to grow the array, do it now.
		if (used + len >= allocated)
			reallocate(used + (len * 2));
		u32 start = used;

		// Convert UTF-8 to UTF-16.
		u32 pos = start;
		for (u32 l = 0; l<len;)
		{
			++used;
			if (((c2[l] >> 6) & 0x03) == 0x02)
			{	// Invalid continuation byte.
				array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
				++l;
			}
			else if (c2[l] == 0xC0 || c2[l] == 0xC1)
			{	// Invalid byte - overlong encoding.
				array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
				++l;
			}
			else if ((c2[l] & 0xF8) == 0xF0)
			{	// 4 bytes UTF-8, 2 bytes UTF-16.
				// Check for a full string.
				if ((l + 3) >= len)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += 3;
					break;
				}

				// Validate.
				bool valid = true;
				u8 l2 = 0;
				if (valid && (((c2[l+1] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (valid && (((c2[l+2] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (valid && (((c2[l+3] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (!valid)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += l2;
					continue;
				}

				// Decode.
				uchar8_t b1 = ((c2[l] & 0x7) << 2) | ((c2[l+1] >> 4) & 0x3);
				uchar8_t b2 = ((c2[l+1] & 0xF) << 4) | ((c2[l+2] >> 2) & 0xF);
				uchar8_t b3 = ((c2[l+2] & 0x3) << 6) | (c2[l+3] & 0x3F);
				uchar32_t v = b3 | ((uchar32_t)b2 << 8) | ((uchar32_t)b1 << 16);

				// Split v up into a surrogate pair.
				uchar16_t x = static_cast<uchar16_t>(v);
				uchar16_t vh = UTF16_HI_SURROGATE | ((((v >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
				uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));

				array[pos++] = vh;
				array[pos++] = vl;
				l += 4;
				++used;		// Using two shorts this time, so increase used by 1.
			}
			else if ((c2[l] & 0xF0) == 0xE0)
			{	// 3 bytes UTF-8, 1 byte UTF-16.
				// Check for a full string.
				if ((l + 2) >= len)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += 2;
					break;
				}

				// Validate.
				bool valid = true;
				u8 l2 = 0;
				if (valid && (((c2[l+1] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (valid && (((c2[l+2] >> 6) & 0x03) == 0x02)) ++l2; else valid = false;
				if (!valid)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += l2;
					continue;
				}

				// Decode.
				uchar8_t b1 = ((c2[l] & 0xF) << 4) | ((c2[l+1] >> 2) & 0xF);
				uchar8_t b2 = ((c2[l+1] & 0x3) << 6) | (c2[l+2] & 0x3F);
				uchar16_t ch = b2 | ((uchar16_t)b1 << 8);
				array[pos++] = ch;
				l += 3;
			}
			else if ((c2[l] & 0xE0) == 0xC0)
			{	// 2 bytes UTF-8, 1 byte UTF-16.
				// Check for a full string.
				if ((l + 1) >= len)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					l += 1;
					break;
				}

				// Validate.
				if (((c2[l+1] >> 6) & 0x03) != 0x02)
				{
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
					++l;
					continue;
				}

				// Decode.
				uchar8_t b1 = (c2[l] >> 2) & 0x7;
				uchar8_t b2 = ((c2[l] & 0x3) << 6) | (c2[l+1] & 0x3F);
				uchar16_t ch = b2 | ((uchar16_t)b1 << 8);
				array[pos++] = ch;
				l += 2;
			}
			else
			{	// 1 byte UTF-8, 1 byte UTF-16.
				// Validate.
				if (c2[l] > 0x7F)
				{	// Values above 0xF4 are restricted and aren't used.  By now, anything above 0x7F is invalid.
					array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
				}
				else array[pos++] = static_cast<uchar16_t>(c2[l]);
				++l;
			}
		}
		array[used] = 0;

		// Validate our new UTF-16 string.
		validate();

		return *this;
	}


	//! Appends a UTF-16 string to this ustring16
	//! \param other The UTF-16 string to append.
	//! \param length The length of the string to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& append(const uchar16_t* const other, u32 length=0xffffffff)
	{
		if (!other)
			return *this;

		// Determine if the string is long enough for a BOM.
		u32 len = 0;
		const uchar16_t* p = other;
		do
		{
			++len;
		} while (*p++ && len < unicode::BOM_ENCODE_UTF16_LEN);

		// Check for the BOM to determine the string's endianness.
		unicode::EUTF_ENDIAN c_end = unicode::EUTFEE_NATIVE;
		if (memcmp(other, unicode::BOM_ENCODE_UTF16_LE, unicode::BOM_ENCODE_UTF16_LEN) == 0)
			c_end = unicode::EUTFEE_LITTLE;
		else if (memcmp(other, unicode::BOM_ENCODE_UTF16_BE, unicode::BOM_ENCODE_UTF16_LEN) == 0)
			c_end = unicode::EUTFEE_BIG;

		// If a BOM was found, don't include it in the string.
		const uchar16_t* c2 = other;
		if (c_end != unicode::EUTFEE_NATIVE)
		{
			c2 = other + unicode::BOM_UTF16_LEN;
			length -= unicode::BOM_UTF16_LEN;
		}

		// Calculate the size of the string to read in.
		len = 0;
		p = c2;
		do
		{
			++len;
		} while(*p++ && len < length);
		if (len > length)
			len = length;

		// If we need to grow the size of the array, do it now.
		if (used + len >= allocated)
			reallocate(used + (len * 2));
		u32 start = used;
		used += len;

		// Copy the string now.
		unicode::EUTF_ENDIAN m_end = getEndianness();
		for (u32 l = start; l < start + len; ++l)
		{
			array[l] = (uchar16_t)c2[l];
			if (c_end != unicode::EUTFEE_NATIVE && c_end != m_end)
				array[l] = unicode::swapEndian16(array[l]);
		}

		array[used] = 0;

		// Validate our new UTF-16 string.
		validate();
		return *this;
	}


	//! Appends a UTF-32 string to this ustring16
	//! \param other The UTF-32 string to append.
	//! \param length The length of the string to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& append(const uchar32_t* const other, u32 length=0xffffffff)
	{
		if (!other)
			return *this;

		// Check for the BOM to determine the string's endianness.
		unicode::EUTF_ENDIAN c_end = unicode::EUTFEE_NATIVE;
		if (memcmp(other, unicode::BOM_ENCODE_UTF32_LE, unicode::BOM_ENCODE_UTF32_LEN) == 0)
			c_end = unicode::EUTFEE_LITTLE;
		else if (memcmp(other, unicode::BOM_ENCODE_UTF32_BE, unicode::BOM_ENCODE_UTF32_LEN) == 0)
			c_end = unicode::EUTFEE_BIG;

		// If a BOM was found, don't include it in the string.
		const uchar32_t* c2 = other;
		if (c_end != unicode::EUTFEE_NATIVE)
		{
			c2 = other + unicode::BOM_UTF32_LEN;
			length -= unicode::BOM_UTF32_LEN;
		}

		// Calculate the size of the string to read in.
		u32 len = 0;
		const uchar32_t* p = c2;
		do
		{
			++len;
		} while(*p++ && len < length);
		if (len > length)
			len = length;

		// If we need to grow the size of the array, do it now.
		// In case all of the UTF-32 string is split into surrogate pairs, do len * 2.
		if (used + (len * 2) >= allocated)
			reallocate(used + ((len * 2) * 2));
		u32 start = used;

		// Convert UTF-32 to UTF-16.
		unicode::EUTF_ENDIAN m_end = getEndianness();
		u32 pos = start;
		for (u32 l = 0; l<len; ++l)
		{
			++used;

			uchar32_t ch = c2[l];
			if (c_end != unicode::EUTFEE_NATIVE && c_end != m_end)
				ch = unicode::swapEndian32(ch);

			if (ch > 0xFFFF)
			{
				// Split ch up into a surrogate pair as it is over 16 bits long.
				uchar16_t x = static_cast<uchar16_t>(ch);
				uchar16_t vh = UTF16_HI_SURROGATE | ((((ch >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
				uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));
				array[pos++] = vh;
				array[pos++] = vl;
				++used;		// Using two shorts, so increased used again.
			}
			else if (ch >= 0xD800 && ch <= 0xDFFF)
			{
				// Between possible UTF-16 surrogates (invalid!)
				array[pos++] = unicode::UTF_REPLACEMENT_CHARACTER;
			}
			else array[pos++] = static_cast<uchar16_t>(ch);
		}
		array[used] = 0;

		// Validate our new UTF-16 string.
		validate();

		return *this;
	}


	//! Appends a ustring16 to this ustring16
	//! \param other The string to append to this one.
	//! \return A reference to our current string.
	ustring16<TAlloc>& append(const ustring16<TAlloc>& other)
	{
		const uchar16_t* oa = other.c_str();

		u32 len = other.size_raw();

		if (used + len >= allocated)
			reallocate(used + len);

		for (u32 l=0; l<len; ++l)
			array[used+l] = oa[l];

		used += len;
		array[used] = 0;

		return *this;
	}


	//! Appends a certain amount of characters of a ustring16 to this ustring16.
	//! \param other The string to append to this one.
	//! \param length How many characters of the other string to add to this one.
	//! \return A reference to our current string.
	ustring16<TAlloc>& append(const ustring16<TAlloc>& other, u32 length)
	{
		if (other.size() == 0)
			return *this;

		if (other.size() < length)
		{
			append(other);
			return *this;
		}

		if (used + length * 2 >= allocated)
			reallocate(used + length * 2);

		const_iterator iter(other, 0);
		u32 l = length;
		while (!iter.atEnd() && l)
		{
			uchar32_t c = *iter;
			append(c);
			++iter;
			--l;
		}

		return *this;
	}


	//! Reserves some memory.
	//! \param count The amount of characters to reserve.
	void reserve(u32 count)
	{
		if (count < allocated)
			return;

		reallocate(count);
	}


	//! Finds first occurrence of character.
	//! \param c The character to search for.
	//! \return Position where the character has been found, or -1 if not found.
	s32 findFirst(uchar32_t c) const
	{
		const_iterator i(*this, 0);

		s32 pos = 0;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			if (c == t)
				return pos;
			++pos;
			++i;
		}

		return -1;
	}

	//! Finds first occurrence of a character of a list.
	//! \param c A list of characters to find. For example if the method should find the first occurrence of 'a' or 'b', this parameter should be "ab".
	//! \param count The amount of characters in the list. Usually, this should be strlen(c).
	//! \return Position where one of the characters has been found, or -1 if not found.
	s32 findFirstChar(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(*this, 0);

		s32 pos = 0;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			for (u32 j=0; j<count; ++j)
				if (t == c[j])
					return pos;
			++pos;
			++i;
		}

		return -1;
	}


	//! Finds first position of a character not in a given list.
	//! \param c A list of characters to NOT find. For example if the method should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
	//! \param count The amount of characters in the list. Usually, this should be strlen(c).
	//! \return Position where the character has been found, or -1 if not found.
	s32 findFirstCharNotInList(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(*this, 0);

		s32 pos = 0;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			u32 j;
			for (j=0; j<count; ++j)
				if (t == c[j])
					break;

			if (j==count)
				return pos;
			++pos;
			++i;
		}

		return -1;
	}

	//! Finds last position of a character not in a given list.
	//! \param c A list of characters to NOT find. For example if the method should find the first occurrence of a character not 'a' or 'b', this parameter should be "ab".
	//! \param count The amount of characters in the list. Usually, this should be strlen(c).
	//! \return Position where the character has been found, or -1 if not found.
	s32 findLastCharNotInList(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(end());
		--i;

		s32 pos = size() - 1;
		while (!i.atStart())
		{
			uchar32_t t = *i;
			u32 j;
			for (j=0; j<count; ++j)
				if (t == c[j])
					break;

			if (j==count)
				return pos;
			--pos;
			--i;
		}

		return -1;
	}

	//! Finds next occurrence of character.
	//! \param c The character to search for.
	//! \param startPos The position in the string to start searching.
	//! \return Position where the character has been found, or -1 if not found.
	s32 findNext(uchar32_t c, u32 startPos) const
	{
		const_iterator i(*this, startPos);

		s32 pos = startPos;
		while (!i.atEnd())
		{
			uchar32_t t = *i;
			if (t == c)
				return pos;
			++pos;
			++i;
		}

		return -1;
	}


	//! Finds last occurrence of character.
	//! \param c The character to search for.
	//! \param start The start position of the reverse search ( default = -1, on end ).
	//! \return Position where the character has been found, or -1 if not found.
	s32 findLast(uchar32_t c, s32 start = -1) const
	{
		u32 s = size();
		start = core::clamp ( start < 0 ? (s32)s : start, 0, (s32)s ) - 1;

		const_iterator i(*this, start);
		u32 pos = start;
		while (!i.atStart())
		{
			uchar32_t t = *i;
			if (t == c)
				return pos;
			--pos;
			--i;
		}

		return -1;
	}

	//! Finds last occurrence of a character in a list.
	//! \param c A list of strings to find. For example if the method should find the last occurrence of 'a' or 'b', this parameter should be "ab".
	//! \param count The amount of characters in the list. Usually, this should be strlen(c).
	//! \return Position where one of the characters has been found, or -1 if not found.
	s32 findLastChar(const uchar32_t* const c, u32 count=1) const
	{
		if (!c || !count)
			return -1;

		const_iterator i(end());
		--i;

		s32 pos = size();
		while (!i.atStart())
		{
			uchar32_t t = *i;
			for (u32 j=0; j<count; ++j)
				if (t == c[j])
					return pos;
			--pos;
			--i;
		}

		return -1;
	}


	//! Finds another ustring16 in this ustring16.
	//! \param str The string to find.
	//! \param start The start position of the search.
	//! \return Positions where the ustring16 has been found, or -1 if not found.
	s32 find(const ustring16<TAlloc>& str, const u32 start = 0) const
	{
		u32 my_size = size();
		u32 their_size = str.size();

		if (their_size == 0 || my_size - start < their_size)
			return -1;

		const_iterator i(*this, start);

		s32 pos = start;
		while (!i.atEnd())
		{
			const_iterator i2(i);
			const_iterator j(str, 0);
			uchar32_t t1 = (uchar32_t)*i2;
			uchar32_t t2 = (uchar32_t)*j;
			while (t1 == t2)
			{
				++i2;
				++j;
				if (j.atEnd())
					return pos;
				t1 = (uchar32_t)*i2;
				t2 = (uchar32_t)*j;
			}
			++i;
			++pos;
		}

		return -1;
	}


	//! Finds another ustring16 in this ustring16.
	//! \param str The string to find.
	//! \param start The start position of the search.
	//! \return Positions where the string has been found, or -1 if not found.
	s32 find_raw(const ustring16<TAlloc>& str, const u32 start = 0) const
	{
		const uchar16_t* data = str.c_str();
		if (data && *data)
		{
			u32 len = 0;

			while (data[len])
				++len;

			if (len > used)
				return -1;

			for (u32 i=start; i<=used-len; ++i)
			{
				u32 j=0;

				while(data[j] && array[i+j] == data[j])
					++j;

				if (!data[j])
					return i;
			}
		}

		return -1;
	}


	//! Returns a substring.
	//! \param begin: Start of substring.
	//! \param length: Length of substring.
	//! \return A reference to our current string.
	ustring16<TAlloc> subString(u32 begin, s32 length) const
	{
		u32 len = size();
		// if start after ustring16
		// or no proper substring length
		if ((length <= 0) || (begin>=len))
			return ustring16<TAlloc>("");
		// clamp length to maximal value
		if ((length+begin) > len)
			length = len-begin;

		ustring16<TAlloc> o;
		o.reserve((length+1) * 2);

		const_iterator i(*this, begin);
		while (!i.atEnd() && length)
		{
			o.append(*i);
			++i;
			--length;
		}

		return o;
	}


	//! Appends a character to this ustring16.
	//! \param c Character to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (char c)
	{
		append((uchar32_t)c);
		return *this;
	}


	//! Appends a character to this ustring16.
	//! \param c Character to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (uchar32_t c)
	{
		append(c);
		return *this;
	}


	//! Appends a number to this ustring16.
	//! \param c Number to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (short c)
	{
		append(core::stringc(c));
		return *this;
	}


	//! Appends a number to this ustring16.
	//! \param c Number to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (unsigned short c)
	{
		append(core::stringc(c));
		return *this;
	}


#ifdef USTRING_CPP0X_NEWLITERALS
	//! Appends a number to this ustring16.
	//! \param c Number to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (int c)
	{
		append(core::stringc(c));
		return *this;
	}


	//! Appends a number to this ustring16.
	//! \param c Number to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (unsigned int c)
	{
		append(core::stringc(c));
		return *this;
	}
#endif


	//! Appends a number to this ustring16.
	//! \param c Number to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (long c)
	{
		append(core::stringc(c));
		return *this;
	}


	//! Appends a number to this ustring16.
	//! \param c Number to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (unsigned long c)
	{
		append(core::stringc(c));
		return *this;
	}


	//! Appends a number to this ustring16.
	//! \param c Number to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (double c)
	{
		append(core::stringc(c));
		return *this;
	}


	//! Appends a char ustring16 to this ustring16.
	//! \param c Char ustring16 to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (const uchar16_t* const c)
	{
		append(c);
		return *this;
	}


	//! Appends a ustring16 to this ustring16.
	//! \param other ustring16 to append.
	//! \return A reference to our current string.
	ustring16<TAlloc>& operator += (const ustring16<TAlloc>& other)
	{
		append(other);
		return *this;
	}


	//! Replaces all characters of a given type with another one.
	//! \param toReplace Character to replace.
	//! \param replaceWith Character replacing the old one.
	//! \return A reference to our current string.
	ustring16<TAlloc>& replace(uchar32_t toReplace, uchar32_t replaceWith)
	{
		iterator i(*this, 0);
		while (!i.atEnd())
		{
			typename ustring16<TAlloc>::access a = *i;
			if ((uchar32_t)a == toReplace)
				a = replaceWith;
			++i;
		}
		return *this;
	}


	//! Replaces all instances of a string with another one.
	//! \param toReplace The string to replace.
	//! \param replaceWith The string replacing the old one.
	//! \return A reference to our current string.
	ustring16<TAlloc>& replace(const ustring16<TAlloc>& toReplace, const ustring16<TAlloc>& replaceWith)
	{
		if (toReplace.size() == 0)
			return *this;

		const uchar16_t* other = toReplace.c_str();
		const uchar16_t* replace = replaceWith.c_str();
		const u32 other_size = toReplace.size_raw();
		const u32 replace_size = replaceWith.size_raw();

		// Determine the delta.  The algorithm will change depending on the delta.
		s32 delta = replace_size - other_size;

		// A character for character replace.  The string will not shrink or grow.
		if (delta == 0)
		{
			s32 pos = 0;
			while ((pos = find_raw(other, pos)) != -1)
			{
				for (u32 i = 0; i < replace_size; ++i)
					array[pos + i] = replace[i];
				++pos;
			}
			return *this;
		}

		// We are going to be removing some characters.  The string will shrink.
		if (delta < 0)
		{
			u32 i = 0;
			for (u32 pos = 0; pos <= used; ++i, ++pos)
			{
				// Is this potentially a match?
				if (array[pos] == *other)
				{
					// Check to see if we have a match.
					u32 j;
					for (j = 0; j < other_size; ++j)
					{
						if (array[pos + j] != other[j])
							break;
					}

					// If we have a match, replace characters.
					if (j == other_size)
					{
						for (j = 0; j < replace_size; ++j)
							array[i + j] = replace[j];
						i += replace_size - 1;
						pos += other_size - 1;
						continue;
					}
				}

				// No match found, just copy characters.
				array[i - 1] = array[pos];
			}
			array[i] = 0;
			used = i;

			return *this;
		}

		// We are going to be adding characters, so the string size will increase.
		// Count the number of times toReplace exists in the string so we can allocate the new size.
		u32 find_count = 0;
		s32 pos = 0;
		while ((pos = find_raw(other, pos)) != -1)
		{
			++find_count;
			++pos;
		}

		// Re-allocate the string now, if needed.
		u32 len = delta * find_count;
		if (used + len >= allocated)
			reallocate(used + len);

		// Start replacing.
		pos = 0;
		while ((pos = find_raw(other, pos)) != -1)
		{
			uchar16_t* start = array + pos + other_size - 1;
			uchar16_t* ptr   = array + used;
			uchar16_t* end   = array + used + delta;

			// Shift characters to make room for the string.
			while (ptr != start)
			{
				*end = *ptr;
				--ptr;
				--end;
			}

			// Add the new string now.
			for (u32 i = 0; i < replace_size; ++i)
				array[pos + i] = replace[i];

			pos += replace_size;
			used += delta;
		}

		// Terminate the string and return ourself.
		array[used] = 0;
		return *this;
	}


	//! Removes characters from a ustring16..
	//! \param c The character to remove.
	//! \return A reference to our current string.
	ustring16<TAlloc>& remove(uchar32_t c)
	{
		u32 pos = 0;
		u32 found = 0;
		u32 len = (c > 0xFFFF ? 2 : 1);		// Remove characters equal to the size of c as a UTF-16 character.
		for (u32 i=0; i<=used; ++i)
		{
			uchar32_t uc32 = 0;
			if (!UTF16_IS_SURROGATE_HI(array[i]))
				uc32 |= array[i];
			else if (i + 1 <= used)
			{
				// Convert the surrogate pair into a single UTF-32 character.
				uc32 = unicode::toUTF32(array[i], array[i + 1]);
			}
			u32 len2 = (uc32 > 0xFFFF ? 2 : 1);

			if (uc32 == c)
			{
				found += len;
				continue;
			}

			array[pos++] = array[i];
			if (len2 == 2)
				array[pos++] = array[++i];
		}
		used -= found;
		array[used] = 0;
		return *this;
	}


	//! Removes a ustring16 from the ustring16.
	//! \param toRemove The string to remove.
	//! \return A reference to our current string.
	ustring16<TAlloc>& remove(const ustring16<TAlloc>& toRemove)
	{
		u32 size = toRemove.size_raw();
		if (size == 0) return *this;

		const uchar16_t* tra = toRemove.c_str();
		u32 pos = 0;
		u32 found = 0;
		for (u32 i=0; i<=used; ++i)
		{
			u32 j = 0;
			while (j < size)
			{
				if (array[i + j] != tra[j])
					break;
				++j;
			}
			if (j == size)
			{
				found += size;
				i += size - 1;
				continue;
			}

			array[pos++] = array[i];
		}
		used -= found;
		array[used] = 0;
		return *this;
	}


	//! Removes characters from the ustring16.
	//! \param characters The characters to remove.
	//! \return A reference to our current string.
	ustring16<TAlloc>& removeChars(const ustring16<TAlloc>& characters)
	{
		if (characters.size_raw() == 0)
			return *this;

		u32 pos = 0;
		u32 found = 0;
		const_iterator iter(characters);
		for (u32 i=0; i<=used; ++i)
		{
			uchar32_t uc32 = 0;
			if (!UTF16_IS_SURROGATE_HI(array[i]))
				uc32 |= array[i];
			else if (i + 1 <= used)
			{
				// Convert the surrogate pair into a single UTF-32 character.
				uc32 = unicode::toUTF32(array[i], array[i+1]);
			}
			u32 len2 = (uc32 > 0xFFFF ? 2 : 1);

			bool cont = false;
			iter.toStart();
			while (!iter.atEnd())
			{
				uchar32_t c = *iter;
				if (uc32 == c)
				{
					found += (c > 0xFFFF ? 2 : 1);		// Remove characters equal to the size of c as a UTF-16 character.
					++i;
					cont = true;
					break;
				}
				++iter;
			}
			if (cont) continue;

			array[pos++] = array[i];
			if (len2 == 2)
				array[pos++] = array[++i];
		}
		used -= found;
		array[used] = 0;
		return *this;
	}


	//! Trims the ustring16.
	//! Removes the specified characters (by default, Latin-1 whitespace) from the begining and the end of the ustring16.
	//! \param whitespace The characters that are to be considered as whitespace.
	//! \return A reference to our current string.
	ustring16<TAlloc>& trim(const ustring16<TAlloc>& whitespace = " \t\n\r")
	{
		core::array<uchar32_t> utf32white = whitespace.toUTF32();

		// find start and end of the substring without the specified characters
		const s32 begin = findFirstCharNotInList(utf32white.const_pointer(), whitespace.used + 1);
		if (begin == -1)
			return (*this="");

		const s32 end = findLastCharNotInList(utf32white.const_pointer(), whitespace.used + 1);

		return (*this = subString(begin, (end +1) - begin));
	}


	//! Erases a character from the ustring16.
	//! May be slow, because all elements following after the erased element have to be copied.
	//! \param index Index of element to be erased.
	//! \return A reference to our current string.
	ustring16<TAlloc>& erase(u32 index)
	{
		_IRR_DEBUG_BREAK_IF(index>used) // access violation

		iterator i(*this, index);

		uchar32_t t = *i;
		u32 len = (t > 0xFFFF ? 2 : 1);

		for (u32 j = static_cast<u32>(i.getPos()) + len; j <= used; ++j)
			array[j - len] = array[j];

		used -= len;
		array[used] = 0;

		return *this;
	}


	//! Validate the existing ustring16, checking for valid surrogate pairs and checking for proper termination.
	//! \return A reference to our current string.
	ustring16<TAlloc>& validate()
	{
		// Validate all unicode characters.
		for (u32 i=0; i<allocated; ++i)
		{
			// Terminate on existing null.
			if (array[i] == 0)
			{
				used = i;
				return *this;
			}
			if (UTF16_IS_SURROGATE(array[i]))
			{
				if (((i+1) >= allocated) || UTF16_IS_SURROGATE_LO(array[i]))
					array[i] = unicode::UTF_REPLACEMENT_CHARACTER;
				else if (UTF16_IS_SURROGATE_HI(array[i]) && !UTF16_IS_SURROGATE_LO(array[i+1]))
					array[i] = unicode::UTF_REPLACEMENT_CHARACTER;
				++i;
			}
			if (array[i] >= 0xFDD0 && array[i] <= 0xFDEF)
				array[i] = unicode::UTF_REPLACEMENT_CHARACTER;
		}

		// terminate
		used = 0;
		if (allocated > 0)
		{
			used = allocated - 1;
			array[used] = 0;
		}
		return *this;
	}


	//! Gets the last char of the ustring16, or 0.
	//! \return The last char of the ustring16, or 0.
	uchar32_t lastChar() const
	{
		if (used < 1)
			return 0;

		if (UTF16_IS_SURROGATE_LO(array[used-1]))
		{
			// Make sure we have a paired surrogate.
			if (used < 2)
				return 0;

			// Check for an invalid surrogate.
			if (!UTF16_IS_SURROGATE_HI(array[used-2]))
				return 0;

			// Convert the surrogate pair into a single UTF-32 character.
			return unicode::toUTF32(array[used-2], array[used-1]);
		}
		else
		{
			return array[used-1];
		}
	}


	//! Split the ustring16 into parts.
	/** This method will split a ustring16 at certain delimiter characters
	into the container passed in as reference. The type of the container
	has to be given as template parameter. It must provide a push_back and
	a size method.
	\param ret The result container
	\param c C-style ustring16 of delimiter characters
	\param count Number of delimiter characters
	\param ignoreEmptyTokens Flag to avoid empty substrings in the result
	container. If two delimiters occur without a character in between, an
	empty substring would be placed in the result. If this flag is set,
	only non-empty strings are stored.
	\param keepSeparators Flag which allows to add the separator to the
	result ustring16. If this flag is true, the concatenation of the
	substrings results in the original ustring16. Otherwise, only the
	characters between the delimiters are returned.
	\return The number of resulting substrings
	*/
	template<class container>
	u32 split(container& ret, const uchar32_t* const c, u32 count=1, bool ignoreEmptyTokens=true, bool keepSeparators=false) const
	{
		if (!c)
			return 0;

		const_iterator i(*this);
		const u32 oldSize=ret.size();
		u32 pos = 0;
		u32 lastpos = 0;
		u32 lastpospos = 0;
		bool lastWasSeparator = false;
		while (!i.atEnd())
		{
			uchar32_t ch = *i;
			bool foundSeparator = false;
			for (u32 j=0; j<count; ++j)
			{
				if (ch == c[j])
				{
					if ((!ignoreEmptyTokens || pos - lastpos != 0) &&
							!lastWasSeparator)
					ret.push_back(ustring16<TAlloc>(&array[lastpospos], pos - lastpos));
					foundSeparator = true;
					lastpos = (keepSeparators ? pos : pos + 1);
					lastpospos = (keepSeparators ? i.getPos() : i.getPos() + 1);
					break;
				}
			}
			lastWasSeparator = foundSeparator;
			++pos;
			++i;
		}
		u32 s = size() + 1;
		if (s > lastpos)
			ret.push_back(ustring16<TAlloc>(&array[lastpospos], s - lastpos));
		return ret.size()-oldSize;
	}


	//! Split the ustring16 into parts.
	/** This method will split a ustring16 at certain delimiter characters
	into the container passed in as reference. The type of the container
	has to be given as template parameter. It must provide a push_back and
	a size method.
	\param ret The result container
	\param c A unicode string of delimiter characters
	\param ignoreEmptyTokens Flag to avoid empty substrings in the result
	container. If two delimiters occur without a character in between, an
	empty substring would be placed in the result. If this flag is set,
	only non-empty strings are stored.
	\param keepSeparators Flag which allows to add the separator to the
	result ustring16. If this flag is true, the concatenation of the
	substrings results in the original ustring16. Otherwise, only the
	characters between the delimiters are returned.
	\return The number of resulting substrings
	*/
	template<class container>
	u32 split(container& ret, const ustring16<TAlloc>& c, bool ignoreEmptyTokens=true, bool keepSeparators=false) const
	{
		core::array<uchar32_t> v = c.toUTF32();
		return split(ret, v.pointer(), v.size(), ignoreEmptyTokens, keepSeparators);
	}


	//! Gets the size of the allocated memory buffer for the string.
	//! \return The size of the allocated memory buffer.
	u32 capacity() const
	{
		return allocated;
	}


	//! Returns the raw number of UTF-16 code points in the string which includes the individual surrogates.
	//! \return The raw number of UTF-16 code points, excluding the trialing NUL.
	u32 size_raw() const
	{
		return used;
	}


	//! Inserts a character into the string.
	//! \param c The character to insert.
	//! \param pos The position to insert the character.
	//! \return A reference to our current string.
	ustring16<TAlloc>& insert(uchar32_t c, u32 pos)
	{
		u8 len = (c > 0xFFFF ? 2 : 1);

		if (used + len >= allocated)
			reallocate(used + len);

		used += len;

		iterator iter(*this, pos);
		for (u32 i = used - 2; i > iter.getPos(); --i)
			array[i] = array[i - len];

		if (c > 0xFFFF)
		{
			// c will be multibyte, so split it up into a surrogate pair.
			uchar16_t x = static_cast<uchar16_t>(c);
			uchar16_t vh = UTF16_HI_SURROGATE | ((((c >> 16) & ((1 << 5) - 1)) - 1) << 6) | (x >> 10);
			uchar16_t vl = UTF16_LO_SURROGATE | (x & ((1 << 10) - 1));
			array[iter.getPos()] = vh;
			array[iter.getPos()+1] = vl;
		}
		else
		{
			array[iter.getPos()] = static_cast<uchar16_t>(c);
		}
		array[used] = 0;
		return *this;
	}


	//! Inserts a string into the string.
	//! \param c The string to insert.
	//! \param pos The position to insert the string.
	//! \return A reference to our current string.
	ustring16<TAlloc>& insert(const ustring16<TAlloc>& c, u32 pos)
	{
		u32 len = c.size_raw();
		if (len == 0) return *this;

		if (used + len >= allocated)
			reallocate(used + len);

		used += len;

		iterator iter(*this, pos);
		for (u32 i = used - 2; i > iter.getPos() + len; --i)
			array[i] = array[i - len];

		const uchar16_t* s = c.c_str();
		for (u32 i = 0; i < len; ++i)
		{
			array[pos++] = *s;
			++s;
		}

		array[used] = 0;
		return *this;
	}


	//! Inserts a character into the string.
	//! \param c The character to insert.
	//! \param pos The position to insert the character.
	//! \return A reference to our current string.
	ustring16<TAlloc>& insert_raw(uchar16_t c, u32 pos)
	{
		if (used + 1 >= allocated)
			reallocate(used + 1);

		++used;

		for (u32 i = used - 1; i > pos; --i)
			array[i] = array[i - 1];

		array[pos] = c;
		array[used] = 0;
		return *this;
	}


	//! Removes a character from string.
	//! \param pos Position of the character to remove.
	//! \return A reference to our current string.
	ustring16<TAlloc>& erase_raw(u32 pos)
	{
		for (u32 i=pos; i<=used; ++i)
		{
			array[i] = array[i + 1];
		}
		--used;
		array[used] = 0;
		return *this;
	}


	//! Replaces a character in the string.
	//! \param c The new character.
	//! \param pos The position of the character to replace.
	//! \return A reference to our current string.
	ustring16<TAlloc>& replace_raw(uchar16_t c, u32 pos)
	{
		array[pos] = c;
		return *this;
	}


	//! Returns an iterator to the beginning of the string.
	//! \return An iterator to the beginning of the string.
	iterator begin()
	{
		iterator i(*this, 0);
		return i;
	}


	//! Returns an iterator to the beginning of the string.
	//! \return An iterator to the beginning of the string.
	const_iterator begin() const
	{
		const_iterator i(*this, 0);
		return i;
	}


	//! Returns an iterator to the beginning of the string.
	//! \return An iterator to the beginning of the string.
	const_iterator cbegin() const
	{
		const_iterator i(*this, 0);
		return i;
	}


	//! Returns an iterator to the end of the string.
	//! \return An iterator to the end of the string.
	iterator end()
	{
		iterator i(*this, 0);
		i.toEnd();
		return i;
	}


	//! Returns an iterator to the end of the string.
	//! \return An iterator to the end of the string.
	const_iterator end() const
	{
		const_iterator i(*this, 0);
		i.toEnd();
		return i;
	}


	//! Returns an iterator to the end of the string.
	//! \return An iterator to the end of the string.
	const_iterator cend() const
	{
		const_iterator i(*this, 0);
		i.toEnd();
		return i;
	}


	//! Converts the string to a UTF-8 encoded string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return A string containing the UTF-8 encoded string.
	core::string<uchar8_t> toUTF8_s(const bool addBOM = false) const
	{
		core::string<uchar8_t> ret;
		ret.reserve(used * 4 + (addBOM ? unicode::BOM_UTF8_LEN : 0) + 1);
		const_iterator iter(*this, 0);

		// Add the byte order mark if the user wants it.
		if (addBOM)
		{
			ret.append(unicode::BOM_ENCODE_UTF8[0]);
			ret.append(unicode::BOM_ENCODE_UTF8[1]);
			ret.append(unicode::BOM_ENCODE_UTF8[2]);
		}

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (c > 0xFFFF)
			{	// 4 bytes
				uchar8_t b1 = (0x1E << 3) | ((c >> 18) & 0x7);
				uchar8_t b2 = (0x2 << 6) | ((c >> 12) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b4 = (0x2 << 6) | (c & 0x3F);
				ret.append(b1);
				ret.append(b2);
				ret.append(b3);
				ret.append(b4);
			}
			else if (c > 0x7FF)
			{	// 3 bytes
				uchar8_t b1 = (0xE << 4) | ((c >> 12) & 0xF);
				uchar8_t b2 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | (c & 0x3F);
				ret.append(b1);
				ret.append(b2);
				ret.append(b3);
			}
			else if (c > 0x7F)
			{	// 2 bytes
				uchar8_t b1 = (0x6 << 5) | ((c >> 6) & 0x1F);
				uchar8_t b2 = (0x2 << 6) | (c & 0x3F);
				ret.append(b1);
				ret.append(b2);
			}
			else
			{	// 1 byte
				ret.append(static_cast<uchar8_t>(c));
			}
			++iter;
		}
		return ret;
	}


	//! Converts the string to a UTF-8 encoded string array.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return An array containing the UTF-8 encoded string.
	core::array<uchar8_t> toUTF8(const bool addBOM = false) const
	{
		core::array<uchar8_t> ret(used * 4 + (addBOM ? unicode::BOM_UTF8_LEN : 0) + 1);
		const_iterator iter(*this, 0);

		// Add the byte order mark if the user wants it.
		if (addBOM)
		{
			ret.push_back(unicode::BOM_ENCODE_UTF8[0]);
			ret.push_back(unicode::BOM_ENCODE_UTF8[1]);
			ret.push_back(unicode::BOM_ENCODE_UTF8[2]);
		}

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (c > 0xFFFF)
			{	// 4 bytes
				uchar8_t b1 = (0x1E << 3) | ((c >> 18) & 0x7);
				uchar8_t b2 = (0x2 << 6) | ((c >> 12) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b4 = (0x2 << 6) | (c & 0x3F);
				ret.push_back(b1);
				ret.push_back(b2);
				ret.push_back(b3);
				ret.push_back(b4);
			}
			else if (c > 0x7FF)
			{	// 3 bytes
				uchar8_t b1 = (0xE << 4) | ((c >> 12) & 0xF);
				uchar8_t b2 = (0x2 << 6) | ((c >> 6) & 0x3F);
				uchar8_t b3 = (0x2 << 6) | (c & 0x3F);
				ret.push_back(b1);
				ret.push_back(b2);
				ret.push_back(b3);
			}
			else if (c > 0x7F)
			{	// 2 bytes
				uchar8_t b1 = (0x6 << 5) | ((c >> 6) & 0x1F);
				uchar8_t b2 = (0x2 << 6) | (c & 0x3F);
				ret.push_back(b1);
				ret.push_back(b2);
			}
			else
			{	// 1 byte
				ret.push_back(static_cast<uchar8_t>(c));
			}
			++iter;
		}
		ret.push_back(0);
		return ret;
	}


#ifdef USTRING_CPP0X_NEWLITERALS	// C++0x
	//! Converts the string to a UTF-16 encoded string.
	//! \param endian The desired endianness of the string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return A string containing the UTF-16 encoded string.
	core::string<char16_t> toUTF16_s(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::string<char16_t> ret;
		ret.reserve(used + (addBOM ? unicode::BOM_UTF16_LEN : 0) + 1);

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				ret[0] = unicode::BOM;
			else if (endian == unicode::EUTFEE_LITTLE)
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ret.c_str());
				*ptr8++ = unicode::BOM_ENCODE_UTF16_LE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_LE[1];
			}
			else
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ret.c_str());
				*ptr8++ = unicode::BOM_ENCODE_UTF16_BE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_BE[1];
			}
		}

		ret.append(array);
		if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
		{
			char16_t* ptr = ret.c_str();
			for (u32 i = 0; i < ret.size(); ++i)
				*ptr++ = unicode::swapEndian16(*ptr);
		}
		return ret;
	}
#endif


	//! Converts the string to a UTF-16 encoded string array.
	//! Unfortunately, no toUTF16_s() version exists due to limitations with Irrlicht's string class.
	//! \param endian The desired endianness of the string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return An array containing the UTF-16 encoded string.
	core::array<uchar16_t> toUTF16(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::array<uchar16_t> ret(used + (addBOM ? unicode::BOM_UTF16_LEN : 0) + 1);
		uchar16_t* ptr = ret.pointer();

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				*ptr = unicode::BOM;
			else if (endian == unicode::EUTFEE_LITTLE)
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ptr);
				*ptr8++ = unicode::BOM_ENCODE_UTF16_LE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_LE[1];
			}
			else
			{
				uchar8_t* ptr8 = reinterpret_cast<uchar8_t*>(ptr);
				*ptr8++ = unicode::BOM_ENCODE_UTF16_BE[0];
				*ptr8 = unicode::BOM_ENCODE_UTF16_BE[1];
			}
			++ptr;
		}

		memcpy((void*)ptr, (void*)array, used * sizeof(uchar16_t));
		if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
		{
			for (u32 i = 0; i <= used; ++i)
				ptr[i] = unicode::swapEndian16(ptr[i]);
		}
		ret.set_used(used + (addBOM ? unicode::BOM_UTF16_LEN : 0));
		ret.push_back(0);
		return ret;
	}


#ifdef USTRING_CPP0X_NEWLITERALS	// C++0x
	//! Converts the string to a UTF-32 encoded string.
	//! \param endian The desired endianness of the string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return A string containing the UTF-32 encoded string.
	core::string<char32_t> toUTF32_s(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::string<char32_t> ret;
		ret.reserve(size() + 1 + (addBOM ? unicode::BOM_UTF32_LEN : 0));
		const_iterator iter(*this, 0);

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				ret.append(unicode::BOM);
			else
			{
				union
				{
					uchar32_t full;
					u8 chunk[4];
				} t;

				if (endian == unicode::EUTFEE_LITTLE)
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_LE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_LE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_LE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_LE[3];
				}
				else
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_BE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_BE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_BE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_BE[3];
				}
				ret.append(t.full);
			}
		}

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
				c = unicode::swapEndian32(c);
			ret.append(c);
			++iter;
		}
		return ret;
	}
#endif


	//! Converts the string to a UTF-32 encoded string array.
	//! Unfortunately, no toUTF32_s() version exists due to limitations with Irrlicht's string class.
	//! \param endian The desired endianness of the string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return An array containing the UTF-32 encoded string.
	core::array<uchar32_t> toUTF32(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		core::array<uchar32_t> ret(size() + (addBOM ? unicode::BOM_UTF32_LEN : 0) + 1);
		const_iterator iter(*this, 0);

		// Add the BOM if specified.
		if (addBOM)
		{
			if (endian == unicode::EUTFEE_NATIVE)
				ret.push_back(unicode::BOM);
			else
			{
				union
				{
					uchar32_t full;
					u8 chunk[4];
				} t;

				if (endian == unicode::EUTFEE_LITTLE)
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_LE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_LE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_LE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_LE[3];
				}
				else
				{
					t.chunk[0] = unicode::BOM_ENCODE_UTF32_BE[0];
					t.chunk[1] = unicode::BOM_ENCODE_UTF32_BE[1];
					t.chunk[2] = unicode::BOM_ENCODE_UTF32_BE[2];
					t.chunk[3] = unicode::BOM_ENCODE_UTF32_BE[3];
				}
				ret.push_back(t.full);
			}
		}
		ret.push_back(0);

		while (!iter.atEnd())
		{
			uchar32_t c = *iter;
			if (endian != unicode::EUTFEE_NATIVE && getEndianness() != endian)
				c = unicode::swapEndian32(c);
			ret.push_back(c);
			++iter;
		}
		return ret;
	}


	//! Converts the string to a wchar_t encoded string.
	/** The size of a wchar_t changes depending on the platform.  This function will store a
	correct UTF-8, -16, or -32 encoded string depending on the size of a wchar_t. **/
	//! \param endian The desired endianness of the string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return A string containing the wchar_t encoded string.
	core::string<wchar_t> toWCHAR_s(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		if (sizeof(wchar_t) == 4)
		{
			core::array<uchar32_t> a(toUTF32(endian, addBOM));
			core::stringw ret(a.pointer());
			return ret;
		}
		else if (sizeof(wchar_t) == 2)
		{
			if (endian == unicode::EUTFEE_NATIVE && addBOM == false)
			{
				core::stringw ret(array);
				return ret;
			}
			else
			{
				core::array<uchar16_t> a(toUTF16(endian, addBOM));
				core::stringw ret(a.pointer());
				return ret;
			}
		}
		else if (sizeof(wchar_t) == 1)
		{
			core::array<uchar8_t> a(toUTF8(addBOM));
			core::stringw ret(a.pointer());
			return ret;
		}

		// Shouldn't happen.
		return core::stringw();
	}


	//! Converts the string to a wchar_t encoded string array.
	/** The size of a wchar_t changes depending on the platform.  This function will store a
	correct UTF-8, -16, or -32 encoded string depending on the size of a wchar_t. **/
	//! \param endian The desired endianness of the string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return An array containing the wchar_t encoded string.
	core::array<wchar_t> toWCHAR(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
		if (sizeof(wchar_t) == 4)
		{
			core::array<uchar32_t> a(toUTF32(endian, addBOM));
			core::array<wchar_t> ret(a.size());
			ret.set_used(a.size());
			memcpy((void*)ret.pointer(), (void*)a.pointer(), a.size() * sizeof(uchar32_t));
			return ret;
		}
		if (sizeof(wchar_t) == 2)
		{
			if (endian == unicode::EUTFEE_NATIVE && addBOM == false)
			{
				core::array<wchar_t> ret(used);
				ret.set_used(used);
				memcpy((void*)ret.pointer(), (void*)array, used * sizeof(uchar16_t));
				return ret;
			}
			else
			{
				core::array<uchar16_t> a(toUTF16(endian, addBOM));
				core::array<wchar_t> ret(a.size());
				ret.set_used(a.size());
				memcpy((void*)ret.pointer(), (void*)a.pointer(), a.size() * sizeof(uchar16_t));
				return ret;
			}
		}
		if (sizeof(wchar_t) == 1)
		{
			core::array<uchar8_t> a(toUTF8(addBOM));
			core::array<wchar_t> ret(a.size());
			ret.set_used(a.size());
			memcpy((void*)ret.pointer(), (void*)a.pointer(), a.size() * sizeof(uchar8_t));
			return ret;
		}

		// Shouldn't happen.
		return core::array<wchar_t>();
	}

	//! Converts the string to a properly encoded io::path string.
	//! \param endian The desired endianness of the string.
	//! \param addBOM If true, the proper unicode byte-order mark will be prefixed to the string.
	//! \return An io::path string containing the properly encoded string.
	io::path toPATH_s(const unicode::EUTF_ENDIAN endian = unicode::EUTFEE_NATIVE, const bool addBOM = false) const
	{
#if defined(_IRR_WCHAR_FILESYSTEM)
		return toWCHAR_s(endian, addBOM);
#else
		return toUTF8_s(addBOM);
#endif
	}

	//! Loads an unknown stream of data.
	//! Will attempt to determine if the stream is unicode data.  Useful for loading from files.
	//! \param data The data stream to load from.
	//! \param data_size The length of the data string.
	//! \return A reference to our current string.
	ustring16<TAlloc>& loadDataStream(const char* data, size_t data_size)
	{
		// Clear our string.
		*this = "";
		if (!data)
			return *this;

		unicode::EUTF_ENCODE e = unicode::determineUnicodeBOM(data);
		switch (e)
		{
			default:
			case unicode::EUTFE_UTF8:
				append((uchar8_t*)data, data_size);
				break;

			case unicode::EUTFE_UTF16:
			case unicode::EUTFE_UTF16_BE:
			case unicode::EUTFE_UTF16_LE:
				append((uchar16_t*)data, data_size / 2);
				break;

			case unicode::EUTFE_UTF32:
			case unicode::EUTFE_UTF32_BE:
			case unicode::EUTFE_UTF32_LE:
				append((uchar32_t*)data, data_size / 4);
				break;
		}

		return *this;
	}

	//! Gets the encoding of the Unicode string this class contains.
	//! \return An enum describing the current encoding of this string.
	const unicode::EUTF_ENCODE getEncoding() const
	{
		return encoding;
	}

	//! Gets the endianness of the Unicode string this class contains.
	//! \return An enum describing the endianness of this string.
	const unicode::EUTF_ENDIAN getEndianness() const
	{
		if (encoding == unicode::EUTFE_UTF16_LE ||
			encoding == unicode::EUTFE_UTF32_LE)
			return unicode::EUTFEE_LITTLE;
		else return unicode::EUTFEE_BIG;
	}

private:

	//! Reallocate the string, making it bigger or smaller.
	//! \param new_size The new size of the string.
	void reallocate(u32 new_size)
	{
		uchar16_t* old_array = array;

		array = allocator.allocate(new_size + 1); //new u16[new_size];
		allocated = new_size + 1;
		if (old_array == 0) return;

		u32 amount = used < new_size ? used : new_size;
		for (u32 i=0; i<=amount; ++i)
			array[i] = old_array[i];

		if (allocated <= used)
			used = allocated - 1;

		array[used] = 0;

		allocator.deallocate(old_array); // delete [] old_array;
	}

	//--- member variables

	uchar16_t* array;
	unicode::EUTF_ENCODE encoding;
	u32 allocated;
	u32 used;
	TAlloc allocator;
	//irrAllocator<uchar16_t> allocator;
};

typedef ustring16<irrAllocator<uchar16_t> > ustring;


//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a null-terminated unicode string.
template <typename TAlloc, class B>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const B* const right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a null-terminated unicode string.
template <class B, typename TAlloc>
inline ustring16<TAlloc> operator+(const B* const left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and an Irrlicht string.
template <typename TAlloc, typename B, typename BAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const string<B, BAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and an Irrlicht string.
template <typename TAlloc, typename B, typename BAlloc>
inline ustring16<TAlloc> operator+(const string<B, BAlloc>& left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a std::basic_string.
template <typename TAlloc, typename B, typename A, typename BAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const std::basic_string<B, A, BAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a std::basic_string.
template <typename TAlloc, typename B, typename A, typename BAlloc>
inline ustring16<TAlloc> operator+(const std::basic_string<B, A, BAlloc>& left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a char.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const char right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a char.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const char left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


#ifdef USTRING_CPP0X_NEWLITERALS
//! Appends a ustring16 and a uchar32_t.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const uchar32_t right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}


//! Appends a ustring16 and a uchar32_t.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const uchar32_t left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret(left);
	ret += right;
	return ret;
}
#endif


//! Appends a ustring16 and a short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const short right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and a short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const short left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


//! Appends a ustring16 and an unsigned short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const unsigned short right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and an unsigned short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const unsigned short left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


//! Appends a ustring16 and an int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const int right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and an int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const int left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


//! Appends a ustring16 and an unsigned int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const unsigned int right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and an unsigned int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const unsigned int left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


//! Appends a ustring16 and a long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const long right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and a long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const long left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


//! Appends a ustring16 and an unsigned long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const unsigned long right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and an unsigned long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const unsigned long left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


//! Appends a ustring16 and a float.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const float right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and a float.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const float left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


//! Appends a ustring16 and a double.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const ustring16<TAlloc>& left, const double right)
{
	ustring16<TAlloc> ret(left);
	ret += core::stringc(right);
	return ret;
}


//! Appends a ustring16 and a double.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const double left, const ustring16<TAlloc>& right)
{
	ustring16<TAlloc> ret((core::stringc(left)));
	ret += right;
	return ret;
}


#ifdef USTRING_CPP0X
//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc>&& operator+(const ustring16<TAlloc>& left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(&, &&)" << std::endl;
	right.insert(left, 0);
	return std::move(right);
}


//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, const ustring16<TAlloc>& right)
{
	//std::cout << "MOVE operator+(&&, &)" << std::endl;
	left.append(right);
	return std::move(left);
}


//! Appends two ustring16s.
template <typename TAlloc>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(&&, &&)" << std::endl;
	if ((right.size_raw() <= left.capacity() - left.size_raw()) ||
		(right.capacity() - right.size_raw() < left.size_raw()))
	{
		left.append(right);
		return std::move(left);
	}
	else
	{
		right.insert(left, 0);
		return std::move(right);
	}
}


//! Appends a ustring16 and a null-terminated unicode string.
template <typename TAlloc, class B>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, const B* const right)
{
	//std::cout << "MOVE operator+(&&, B*)" << std::endl;
	left.append(right);
	return std::move(left);
}


//! Appends a ustring16 and a null-terminated unicode string.
template <class B, typename TAlloc>
inline ustring16<TAlloc>&& operator+(const B* const left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(B*, &&)" << std::endl;
	right.insert(left, 0);
	return std::move(right);
}


//! Appends a ustring16 and an Irrlicht string.
template <typename TAlloc, typename B, typename BAlloc>
inline ustring16<TAlloc>&& operator+(const string<B, BAlloc>& left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(&, &&)" << std::endl;
	right.insert(left, 0);
	return std::move(right);
}


//! Appends a ustring16 and an Irrlicht string.
template <typename TAlloc, typename B, typename BAlloc>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, const string<B, BAlloc>& right)
{
	//std::cout << "MOVE operator+(&&, &)" << std::endl;
	left.append(right);
	return std::move(left);
}


//! Appends a ustring16 and a std::basic_string.
template <typename TAlloc, typename B, typename A, typename BAlloc>
inline ustring16<TAlloc>&& operator+(const std::basic_string<B, A, BAlloc>& left, ustring16<TAlloc>&& right)
{
	//std::cout << "MOVE operator+(&, &&)" << std::endl;
	right.insert(core::ustring16<TAlloc>(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and a std::basic_string.
template <typename TAlloc, typename B, typename A, typename BAlloc>
inline ustring16<TAlloc>&& operator+(ustring16<TAlloc>&& left, const std::basic_string<B, A, BAlloc>& right)
{
	//std::cout << "MOVE operator+(&&, &)" << std::endl;
	left.append(right);
	return std::move(left);
}


//! Appends a ustring16 and a char.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const char right)
{
	left.append((uchar32_t)right);
	return std::move(left);
}


//! Appends a ustring16 and a char.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const char left, ustring16<TAlloc>&& right)
{
	right.insert((uchar32_t)left, 0);
	return std::move(right);
}


#ifdef USTRING_CPP0X_NEWLITERALS
//! Appends a ustring16 and a uchar32_t.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const uchar32_t right)
{
	left.append(right);
	return std::move(left);
}


//! Appends a ustring16 and a uchar32_t.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const uchar32_t left, ustring16<TAlloc>&& right)
{
	right.insert(left, 0);
	return std::move(right);
}
#endif


//! Appends a ustring16 and a short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const short right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and a short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const short left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and an unsigned short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const unsigned short right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and an unsigned short.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const unsigned short left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and an int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const int right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and an int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const int left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and an unsigned int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const unsigned int right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and an unsigned int.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const unsigned int left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and a long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const long right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and a long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const long left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and an unsigned long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const unsigned long right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and an unsigned long.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const unsigned long left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and a float.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const float right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and a float.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const float left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}


//! Appends a ustring16 and a double.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(ustring16<TAlloc>&& left, const double right)
{
	left.append(core::stringc(right));
	return std::move(left);
}


//! Appends a ustring16 and a double.
template <typename TAlloc>
inline ustring16<TAlloc> operator+(const double left, ustring16<TAlloc>&& right)
{
	right.insert(core::stringc(left), 0);
	return std::move(right);
}
#endif


#ifndef USTRING_NO_STL
//! Writes a ustring16 to an ostream.
template <typename TAlloc>
inline std::ostream& operator<<(std::ostream& out, const ustring16<TAlloc>& in)
{
	out << in.toUTF8_s().c_str();
	return out;
}

//! Writes a ustring16 to a wostream.
template <typename TAlloc>
inline std::wostream& operator<<(std::wostream& out, const ustring16<TAlloc>& in)
{
	out << in.toWCHAR_s().c_str();
	return out;
}
#endif


#ifndef USTRING_NO_STL

namespace unicode
{

//! Hashing algorithm for hashing a ustring.  Used for things like unordered_maps.
//! Algorithm taken from std::hash<std::string>.
class hash : public std::unary_function<core::ustring, size_t>
{
	public:
		size_t operator()(const core::ustring& s) const
		{
			size_t ret = 2166136261U;
			size_t index = 0;
			size_t stride = 1 + s.size_raw() / 10;

			core::ustring::const_iterator i = s.begin();
			while (i != s.end())
			{
				// TODO: Don't force u32 on an x64 OS.  Make it agnostic.
				ret = 16777619U * ret ^ (size_t)s[(u32)index];
				index += stride;
				i += stride;
			}
			return (ret);
		}
};

} // end namespace unicode

#endif

} // end namespace core
} // end namespace irr

#endif

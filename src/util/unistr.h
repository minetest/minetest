/*
Minetest
Copyright (C) 2022 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

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

#include <unicode/locid.h>
#include <unicode/unistr.h>

#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <unordered_set>

// Required method for ICU namespace forward declarations due to versioned namespaces
U_NAMESPACE_BEGIN
	class BreakIterator;
	class Collator;
	class StringSearch;
U_NAMESPACE_END

struct UText;

namespace uni
{
	/** An exception indicating that a Unicode error occurred. It indicates one
	  * of two things: either the user used a method improperly, such as mixing
	  * different locales in the same operation, or ICU encountered an
	  * unrecoverable error that we can't handle.
	  *
	  * Generally, this exception should not be handled except to rethrow it as
	  * a more appropriate exception (such as a LuaError if it occurred in some
	  * Lua bindings).
	  */
	class UnicodeException : public std::exception
	{
	private:
		const char *m_message;

	public:
		UnicodeException(const char *message)
		{
			m_message = message;
		}

		virtual const char *what() const noexcept override
		{
			return m_message;
		}
	};

	template<typename T>
	struct ICUDeleter
	{
		void operator()(T *to_delete);
	};

	template<typename T>
	using ICUUniquePtr = std::unique_ptr<T, ICUDeleter<T>>;

	/** Represents a single Unicode codepoint. It is defined as a signed 32-bit
	  * integer as opposed to unsigned or `char32_t` so that `-1` can act as a
	  * sentinel value when a codepoint is invalid for some reason or another.
	  */
	typedef int32_t Code;

	/** Returned by functions to indicate that a Unicode codepoint is invalid.
	  * It is NOT a valid codepoint itself and should not appear in actual
	  * strings. It is distinct from the Unicode replacement codepoint
	  * (`uni::REPLACE_CODE`) so that actual replacement codepoints and invalid
	  * codepoints can be differentiated.
	  */
	const Code INVALID_CODE = -1;

	/** The Unicode replacement codepoint, which is meant to be used to replace
	  * any invalid sequences of codepoints. All Unicode functions that
	  * encounter actual invalid sequences of codepoints will return
	  * `uni::INVALID_CODE`, whereas this codepoint is for user code that needs
	  * to handle invalid codepoints.
	  */
	const Code REPLACE_CODE = 0xFFFD;

	/** Gets the full name of a Unicode codepoint as a string. The name is
	  * written in uppercase.
	  *
	  * \param code The codepoint to get the name of.
	  * \return The name of the codepoint, or "UNKNOWN" if the codepoint is
	  *         unknown or invalid.
	  */
	std::string get_code_name(Code code);

	/** Gets the general category of a Unicode codepoint as the two-character
	  * literal. If the codepoint is unknown or invalid, returns the unknown
	  * category "Cn".
	  *
	  * \param code The codepoint to get the general category of.
	  * \return A two character literal representing the general category, which
	  *         is one of the values specified in
	  *         <https://www.unicode.org/reports/tr44/#General_Category_Values>.
	  */
	const char *get_char_type(Code code);

	/** An iterator for iterating over codepoints in raw UTF-8 text. It only
	  * stores a pointer to the text, so the iterator is only valid as long as
	  * the underlying text is not changed or freed.
	  *
	  * Since this iterator only allows iterating over codepoints, it is meant
	  * to be a lightweight alternative to Str for things that only need the
	  * most rudimentary of Unicode operations. Anything more complex should use
	  * Str and StrIt.
	  *
	  * The iterator is similar to most standard C++ iterators in that the
	  * starting position is at the start of the string and the ending position
	  * is one past the end of the string, or the size of the string.
	  *
	  * The public interface of UTF8It is almost a subset of StrIt, but also
	  * includes functions for moving the iterator directly: getIndex(),
	  * setIndex(), and getEndIndex().
	  */
	class UTF8It
	{
	private:
		/** The UText for iterating over the codepoints. This is preferable to
		  * using the `U8_` macros to move around because it does more extensive
		  * bounds and error checking and properly handles cases of the iterator
		  * being in the middle of a codepoint.
		  */
		ICUUniquePtr<UText> m_iterator;

		/** The current position of the iterator. UText stores the iteration
		  * position itself, but storing it separately here allows setIndex() to
		  * place iterators in the middle of a codepoint without UText
		  * automatically rewinding iteration to the start of the codepoint.
		  */
		size_t m_index;

	public:
		/** Create a new UTF-8 iterator pointing to a null-terminated string.
		  * The size will be found by the iterator. The iterator will start at
		  * the beginning of the string.
		  *
		  * \param str The string to iterate over.
		  */
		UTF8It(const char *str);

		/** Create a new UTF-8 iterator pointing to a string with a specified
		  * size given. The iterator will start at the beginning of the string.
		  *
		  * \param str  The string to iterate over.
		  * \param size The size of the string or substring to iterate over.
		  */
		UTF8It(const char *str, size_t size);

		/** Create a new UTF-8 iterator pointing to the contents of the string
		  * provided. The iterator will start at the beginning of the string.
		  *
		  * \param str The string to iterate over.
		  */
		UTF8It(const std::string &str) :
			UTF8It(str.c_str(), str.size())
		{}

		/** Creates a copy of another UTF-8 iterator.
		  *
		  * \param other The iterator to make a copy of.
		  */
		UTF8It(const UTF8It &other);

		/** Gets the code at the current position. If the iterator started out
		  * in the middle of a codepoint (generally by moving with setIndex()), it
		  * will be moved back to the start of the codepoint.
		  *
		  * Generally, nextCode() and prevCode() are more useful when iterating
		  * over a string in order.
		  *
		  * \return The codepoint at that position, or INVALID_CODE if the
		  *         codepoint is invalid.
		  */
		Code getCode();

		/** Advances the iterator to the next codepoint and returns the
		  * codepoint it was just at. It does not matter whether the iterator is
		  * at the start or in the middle of a codepoint initially.
		  *
		  * \return The codepoint at the position just moved over, or
		  *         INVALID_CODE if the codepoint is invalid or the iterator is
		  *         at the end of the string.
		  */
		Code nextCode();

		/** If the iterator is at the start of a codepoint, moves the iterator
		  * to the start of the prev codepoint. If the iterator is in the middle
		  * of a codepoint, it behaves the same as getCode().
		  *
		  * \return The first codepoint at a position before the current
		  *         iteration position, or INVALID_CODE if that codepoint is
		  *         invalid or the iterator is at the start of the string.
		  */
		Code prevCode();

		/** Queries whether the iterator is at the start of the string.
		  *
		  * \return True if the iterator is at the start of the string, and
		  *         false otherwise.
		  */
		bool isAtStart() const
		{
			return m_index == 0;
		}

		/** Queries whether the iterator is at the end of the string.
		  *
		  * \return True if the iterator is at the end of the string, and false
		  *         otherwise.
		  */
		bool isAtEnd() const
		{
			return m_index == getEndIndex();
		}

		/** Moves the iterator directly to the start of the string. Equivalent
		  * to setIndex(0).
		  */
		void toStart()
		{
			m_index = 0;
		}

		/** Moves the iterator directly to the end of the string. Equivalent to
		  * setIndex(getEndIndex());
		  */
		void toEnd()
		{
			m_index = getEndIndex();
		}

		/** Gets the current position of the iterator as an index into the
		  * string. It does not count codepoints, but bytes.
		  *
		  * \return The current string index of the iterator.
		  */
		size_t getIndex() const
		{
			return m_index;
		}

		/** Sets the current position of the iterator. It may set the iterator
		  * to the middle of a codepoint.
		  *
		  * \param index The string index to move the iterator to.
		  */
		void setIndex(size_t index)
		{
			m_index = index;
		}

		/** Queries the position of an iterator to the end of the string (i.e.
		  * an iterator moved to the end with toEnd()), which is equivalent to
		  * the string size.
		  *
		  * \return The string index of the end iterator position.
		  */
		size_t getEndIndex() const;

		/** Queries whether two iterators are equal, which is true if the
		  * underlying string is equal and both iterators point to the same
		  * position.
		  */
		friend bool operator==(const UTF8It &left, const UTF8It &right);

		/** Same as operator==, but for inequality. */
		friend bool operator!=(const UTF8It &left, const UTF8It &right)
		{
			return !(left == right);
		}

		/** Compares whether one iterator is at an earlier position than
		  * another, which is equivalent to just comparing the positions of the
		  * iterators. If the iterators point to different strings, the value
		  * will not be meaningful.
		  */
		friend bool operator<(const UTF8It &left, const UTF8It &right)
		{
			return left.m_index < right.m_index;
		}

		/** Same as operator<, but for checking if the first iterator is at a
		  * later position than the first.
		  */
		friend bool operator>(const UTF8It &left, const UTF8It &right)
		{
			return left.m_index > right.m_index;
		}

		/** Same as operator<, but for checking if the first iterator is at an
		  * earlier or equal position to the first.
		  */
		friend bool operator<=(const UTF8It &left, const UTF8It &right)
		{
			return left.m_index <= right.m_index;
		}

		/** Same as operator<, but for checking if the first iterator is at a
		  * later or equal position to the first.
		  */
		friend bool operator>=(const UTF8It &left, const UTF8It &right)
		{
			return left.m_index >= right.m_index;
		}
	};

	/** Converts a codepoint to a UTF-8 string containing that codepoint, which
	  * is 2-5 bytes (including the null terminator). The function returns a
	  * value indicating whether or not the operation succeeded.
	  *
	  * \param code The codepoint to convert to a UTF-8 string.
	  * \param str  The buffer where the codepoint will be stored. It is cleared
	  *             to '\0' before writing the codepoint. The value is
	  *             unspecified if the function encounters an error.
	  * \return True if the function encountered an error, false on success.
	  */
	bool code_to_utf8(Code code, char str[5]);

	/** Represents a locale somewhere in the world, consisting of three
	  * components: language, script, and country. Each component can be left
	  * out, in which case it falls back to an unspecified default value.
	  *
	  * Locales can be specified as strings, which have a syntax of the
	  * following:
	  *
	  *     [language][_[script]][_[country]]
	  *
	  * All three components are case-insensitive, and dashes can optionally be
	  * used instead of underscores.
	  *
	  * Some examples of valid locale strings:
	  * - en_US
	  * - es__ES
	  * - fr-ca
	  * - sy_Cyrl_YU
	  * - SR-LATN
	  * - US
	  * - __US
	  *
	  * Locales have a normalized form, which means that proper capitalization
	  * is applied, underscores are used instead of dashes, and all optional
	  * leading underscores are included.
	  */
	class Locale
	{
	private:
		/** The internal ICU locale doing the actual work, which is also passed
		  * to all ICU services using this Locale object.
		  */
		icu::Locale m_locale;

	public:
		/** Creates a new locale that uses the user's default locale.
		  */
		Locale()
		{
			setNormalized(icu::Locale());
		};

		/** Creates a new locale based on a locale string described in the main
		  * documentation for Locale. Using a locale string with a different
		  * format will result in an unspecified locale.
		  *
		  * \param The locale string to create the locale from.
		  */
		Locale(const char *locale)
		{
			setNormalized(icu::Locale::createFromName(locale));
		}

		/** Converts this locale to normalized string form.
		  *
		  * \return The normalized locale string.
		  */
		const char *toString() const
		{
			return m_locale.getName();
		}

		/** Gets the language component of this locale, or an empty string if
		  * none was specified.
		  *
		  * \return The language component of this locale.
		  */
		const char *getLanguage() const
		{
			return m_locale.getLanguage();
		}

		/** Gets the script component of this locale, or an empty string if none
		  * was specified.
		  *
		  * \return The script component of this locale.
		  */
		const char *getScript() const
		{
			return m_locale.getScript();
		}

		/** Gets the country component of this locale, or an empty string if
		  * none was specified.
		  *
		  * \return The country component of this locale.
		  */
		const char *getCountry() const
		{
			return m_locale.getCountry();
		}

		/** Checks if two locales are equal, which is true if they have the same
		  * components.
		  */
		friend bool operator==(const Locale &left, const Locale &right)
		{
			return left.m_locale == right.m_locale;
		}

		/** Checks if two locales are not equal, which is true if at least one
		  * component differs.
		  */
		friend bool operator!=(const Locale &left, const Locale &right)
		{
			return !(left == right);
		}

	private:
		/** Given an ICU locale, strips all information such as variant codes
		  * or attributes. This makes sure that none of those can creep in from
		  * the user's default locale or user input that includes them.
		  *
		  * The reason that this is necessary is that many Str operations take
		  * other Strs, and those operations require that the locales of those
		  * strings are identical. If the default locale had a variant, it would
		  * automatically be different from every other locale even if it had
		  * the same language, script, and country, which would be extremely
		  * confusing and lead to subtle bugs.
		  *
		  * \param locale The locale to normalize and set to m_locale.
		  */
		void setNormalized(const icu::Locale &locale);

		// Start friend interface
		friend class Collator;
		friend class Searcher;
		friend class Str;

		Locale(const icu::Locale &locale) :
			m_locale(locale)
		{}

		/** Returns the interal ICU locale contained in this Locale wrapper.
		  *
		  * \return The internal ICU locale.
		  */
		const icu::Locale &getInternal() const
		{
			return m_locale;
		}
		// End friend interface
	};

	class Str;

	class StrPositioner
	{
	protected:
		std::shared_ptr<const Str> m_str;

	public:
		StrPositioner(std::shared_ptr<const Str> str);

		StrPositioner(const StrPositioner &other);

		StrPositioner &operator=(const StrPositioner &other);

		virtual ~StrPositioner();

	protected:
		// Start friend interface
		friend class Str;

		virtual void updatePositioner()
		{
			// Do nothing by default; subclasses may override if they need to do
			// something when their iterators change.
		};
		// End friend interface
	};

	/** An iterator for iterating over a Str. Besides iterating over plain
	  * codepoints, StrIt can iterate over characters, words, and line breaks.
	  * Like Str, StrIt hides the encoding of the string, so there is no way to
	  * extract individual code units, only codepoints or bigger.
	  *
	  * The iterator is similar to most standard C++ iterators in that the
	  * starting position is at the start of the string and the ending position
	  * is one past the end of the string.
	  *
	  * The public interface of StrIt is a near superset of UTF8It, but since
	  * it hides the encoding, there are no public functions getIndex(),
	  * setIndex(), and getEndIndex() in the public interface as they are very
	  * much tied to the encoding used and encourage bad Unicode practice.
	  *
	  * StrIt stores a pointer to the parent Str and calls methods on it,
	  * including at destruction. Therefore, StrIts **must not** outlive their
	  * parent Strs.
	  *
	  * Unlike most iterators, StrIts are highly stable. When the underlying
	  * Str is changed, the StrIts pointing to it move so that they stay valid
	  * and point to the same position in the surrounding text as much as
	  * possible. For example, assume there is the following text where |
	  * indicates an iterator position rather than a character:
	  *
	  *     chee|se is |ve|ry| tas|ty
	  *         1      2  3  4    5
	  *
	  * Assume "very" is replaced with "extremely" with Str::replace() via the
	  * second and fourth iterators:
	  *
	  *     chee|se is |extremely|| tas|ty
	  *         1      2         34    5
	  *
	  * Notice how iterators 1, 2, 4, and 5 stay in the same position relative
	  * to the surrounding text. Iterator 3, since it is inside the changed text
	  * area, moves to the end of the replacement text. Most Str methods behave
	  * in a similar way, and document exactly how iterators move.
	  *
	  * Some Str methods that modify the string cannot move iterators in a
	  * proper way because they modify the string's internal representation
	  * beyond recognition, such as toUpper() or normalize(). These functions
	  * always move all iterators to the end of the text.
	  */
	class StrIt : public StrPositioner
	{
	private:
		size_t m_index;

	public:
		StrIt(const StrIt &other) :
			StrPositioner(other),
			m_index(other.m_index)
		{}

		Code getCode();

		Code nextCode();

		Code prevCode();

		void nextChar();

		void prevChar();

		void nextWord();

		void prevWord();

		bool nextBreak();

		bool prevBreak();

		void nextLine();

		void prevLine();

		bool isAtStart() const;

		bool isAtEnd() const;

		void toStart();

		void toEnd();

		friend bool operator==(const StrIt &left, const StrIt &right)
		{
			return left.m_str == right.m_str && left.m_index == right.m_index;
		}

		friend bool operator!=(const StrIt &left, const StrIt &right)
		{
			return !(left == right);
		}

		friend bool operator<(const StrIt &left, const StrIt &right)
		{
			return left.m_index < right.m_index;
		}

		friend bool operator>(const StrIt &left, const StrIt &right)
		{
			return left.m_index > right.m_index;
		}

		friend bool operator<=(const StrIt &left, const StrIt &right)
		{
			return left.m_index <= right.m_index;
		}

		friend bool operator>=(const StrIt &left, const StrIt &right)
		{
			return left.m_index >= right.m_index;
		}

	private:
		// Start friend interface
		friend class Searcher;
		friend class Str;

		size_t getIndex() const
		{
			return m_index;
		}

		void setIndex(size_t index)
		{
			m_index = index;
		}
		// End Searcher friend interface

		StrIt(std::shared_ptr<const Str> str, size_t index) :
			StrPositioner(str),
			m_index(index)
		{}
		// End Str friend interface

		void correctNext();

		void correctPrev();

		bool getLineBreakType(icu::BreakIterator *breaker) const;
	};

	/** Str is the locus of Unicode operations save that of layout and
	  * rendering. It stores a string of text and the locale that the text is
	  * in.
	  *
	  * Unlike most string types, which have direct indexing, Str only has
	  * iteration with StrIt. The reason is that, in Unicode, direct indexing
	  * is mostly worthless since it only addresses either code units (which are
	  * almost always worthless) or codepoints (which have few uses as well;
	  * codepoints are often mistaken for characters). Additionally, Str is
	  * meant to be encoding-independent in its external interface: it could be
	  * UTF-8, UTF-16, or UTF-32 without the user code being able to tell.
	  * Therefore, direct indexing is left out as because it has few uses and
	  * encourages misuse of Unicode.
	  *
	  * Str caches all data structures that perform complex Unicode analysis,
	  * namely break iterators for StrIt, because they can be expensive to
	  * create and they take memory, so they are created on an as-needed basis
	  * and cached for as long as possible, only regenerating or changing when
	  * the text or locale has changed.
	  */
	class Str : public std::enable_shared_from_this<Str>
	{
	private:
		icu::UnicodeString m_string;

		Locale m_locale;

		mutable std::unordered_set<StrPositioner *> m_positioners;

		mutable ICUUniquePtr<UText> m_code_breaker;
		mutable ICUUniquePtr<icu::BreakIterator> m_char_breaker;
		mutable ICUUniquePtr<icu::BreakIterator> m_word_breaker;
		mutable ICUUniquePtr<icu::BreakIterator> m_line_breaker;

	public:
		template<typename ...Args>
		static std::shared_ptr<Str> create(Args &&...args)
		{
			return std::make_shared<Str>(std::forward<Args>(args)...);
		}

		static std::shared_ptr<Str> create(std::shared_ptr<Str> other)
		{
			return std::make_shared<Str>(*other);
		}

		Str(const Locale &locale = Locale());

		Str(const char *str, const Locale &locale = Locale());

		Str(const std::string &str, const Locale &locale = Locale());

		Str(Code code, const Locale &locale = Locale()) :
			m_string(code == INVALID_CODE ? REPLACE_CODE : code),
			m_locale(locale)
		{
			regenerate();
		}

		Str(const Str &other) :
			m_string(other.m_string),
			m_locale(other.m_locale)
		{
			regenerate();
		}

		Str &operator=(const Str &other);

		std::string toString() const
		{
			std::string out;
			return m_string.toUTF8String(out);
		}

		bool isEmpty() const
		{
			return m_string.isEmpty();
		}

		friend bool operator==(const Str &left, const Str &right)
		{
			return left.m_string == right.m_string && left.m_locale == right.m_locale;
		}

		friend bool operator!=(const Str &left, const Str &right)
		{
			return !(left == right);
		}

		StrIt itStart() const
		{
			return StrIt(shared_from_this(), 0);
		}

		StrIt itEnd() const
		{
			return StrIt(shared_from_this(), size());
		}

		std::shared_ptr<Str> sub(const StrIt &begin, const StrIt &end) const;

		void append(std::shared_ptr<Str> str);

		void insert(const StrIt &pos, std::shared_ptr<Str> str);

		void replace(const StrIt &begin, const StrIt &end, std::shared_ptr<Str> str);

		void remove(const StrIt &begin, const StrIt &end);

		void clear();

		void trim();

		void toLower();

		void toUpper();

		void toTitle(bool first_only = false);

		void normalize(bool compose = false);

		const Locale &getLocale() const
		{
			return m_locale;
		}

		void setLocale(const Locale &locale);

	private:
		void regenerate();

		void updateBreakers();

		void updatePositioners();

		void updateArea(size_t start, size_t old_end, size_t new_end);

		void updateEntire();

		// Start friend interface
		friend class StrPositioner;

		void registerPositioner(StrPositioner *it) const
		{
			m_positioners.insert(it);
		}

		void unregisterPositioner(StrPositioner *it) const
		{
			m_positioners.erase(it);
		}

		size_t size() const
		{
			return m_string.length();
		}
		// End friend interface

		typedef icu::BreakIterator *(*BreakerCreator)(const icu::Locale &, UErrorCode &);

		icu::BreakIterator *getBreaker(ICUUniquePtr<icu::BreakIterator> &breaker,
				BreakerCreator creator) const;

		// Start friend interface
		friend class Searcher;
		friend class StrIt;

		UText *getCodeBreaker() const
		{
			return m_code_breaker.get();
		}

		icu::BreakIterator *getCharBreaker() const;

		icu::BreakIterator *getWordBreaker() const;

		icu::BreakIterator *getLineBreaker() const;
		// End StrIt friend interface

		friend class Collator;

		const icu::UnicodeString &getInternal() const
		{
			return m_string;
		}
		// End Collator/Searcher friend interface
	};

	class Collator
	{
	public:
		/** Bitmask options that control the collation behaviour. */
		enum Options
		{
			NORMAL = 0,

			/** Indicates that case should be ignored. For example, with this
			  * set, a and A are compared as equal. By default, they are
			  * compared as different.
			  */
			IGNORE_CASE = 0x1,

			/** Indicates that diacritics that are insignificant according to
			  * the given locale should be ignored. For example, in English, A
			  * and Å are compared equal with this set, but they are different
			  * when it is not set. However, in a language like Danish where the
			  * letter Å is treated as a distinct letter, A will not compare
			  * equal to Å no matter what.
			  */
			IGNORE_DIACRITICS = 0x2,

			/** Increases the importance of uppercase letters such that, when
			  * there are no more important differences in the text, uppercase
			  * letters are sorted before lowercase.
			  *
			  * If neither this nor LOWERCASE_FIRST is set, the convention for
			  * the current locale is used. If both are set, UPPERCASE_FIRST
			  * takes precedence.
			  */
			UPPERCASE_FIRST = 0x4,

			/** Increases the importance of lowercase letters such that, when
			  * there are no more important differences in the text, lowercase
			  * letters are sorted before uppercase. See UPPERCASE_FIRST for
			  * more information.
			  */
			LOWERCASE_FIRST = 0x8,
		};

	private:
		ICUUniquePtr<icu::Collator> m_collator;

		Locale m_locale;
		Options m_options;

	public:
		Collator(const Locale &locale, Options options);

		const Locale &getLocale()
		{
			return m_locale;
		}

		void setLocale(const Locale &locale)
		{
			m_locale = locale;
			regenerate();
		}

		Options getOptions()
		{
			return m_options;
		}

		void setOptions(Options options)
		{
			m_options = options;
			configure();
		}

		int collate(std::shared_ptr<Str> left, std::shared_ptr<Str> right) const;

	private:
		void regenerate();

		void configure();
	};

	class Searcher : public StrPositioner
	{
	public:
		/** Bitmask options that control the searching behaviour. Additionally,
		  * Collator::IGNORE_CASE and Collator::IGNORE_DIACRITICS are also valid
		  * bitmasks for anything taking a Searcher::Options.
		  */
		enum Options
		{
			NORMAL = 0,

			/** Specifies whether matches in the searched text may overlap or
			  * not. For instance, with this set, "aa" will match "aaa" twice,
			  * whereas it would only match once without this.
			  */
			OVERLAP = 0x4,

			/** Indicates that the search should start searching again from the
			  * other side when hitting the start or end of the string, rather
			  * than just stopping.
			  */
			WRAP_AROUND = 0x8,

			/** Specifies that the search should only match whole words. Words
			  * are the same as words found by StrIt::nextWord().
			  */
			WHOLE_WORDS_ONLY = 0x10,
		};

	private:
		ICUUniquePtr<icu::StringSearch> m_searcher;

		std::shared_ptr<Str> m_pattern;

		Options m_options;

		StrIt m_start;
		StrIt m_end;

		bool m_needs_regen;

	public:
		Searcher(std::shared_ptr<const Str> str, std::shared_ptr<Str> pattern,
				Options options);

		std::shared_ptr<Str> getPattern() const
		{
			return m_pattern;
		}

		void setPattern(std::shared_ptr<Str> pattern)
		{
			m_pattern = uni::Str::create(pattern);
			updatePositioner();
		}

		Options getOptions()
		{
			return m_options;
		}

		void setOptions(Options options)
		{
			m_options = options;
			updatePositioner();
		}

		bool hasMatch()
		{
			// Regenerating isn't necessary since two indentical iterators will
			// still be identical even if the string changes.
			return m_start != m_end;
		}

		StrIt getMatchStart()
		{
			return m_start;
		}

		StrIt getMatchEnd()
		{
			return m_end;
		}

		void setPos(const StrIt &it)
		{
			m_start = it;
			m_end = it;
		}

		void toStart()
		{
			setPos(m_str->itStart());
		}

		void toEnd()
		{
			setPos(m_str->itEnd());
		}

		bool next();
		bool prev();

		bool following(const StrIt &it)
		{
			setPos(it);
			return next();
		}

		bool preceding(const StrIt &it)
		{
			setPos(it);
			return prev();
		}

	protected:
		virtual void updatePositioner() override
		{
			m_searcher = nullptr;
			m_needs_regen = true;
		}

	private:
		void regenerate();

		void checkRegenerate()
		{
			if (m_needs_regen)
				regenerate();
		}
	};
}

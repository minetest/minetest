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

#include "unistr.h"

#include <unicode/brkiter.h>
#include <unicode/coll.h>
#include <unicode/localebuilder.h>
#include <unicode/normalizer2.h>
#include <unicode/stringoptions.h>
#include <unicode/stsearch.h>
#include <unicode/uchar.h>
#include <unicode/ucol.h>
#include <unicode/usearch.h>
#include <unicode/utext.h>
#include <unicode/utypes.h>

#include <cstring>

/** Checks an ICU status code for failure, throwing a UnicodeException with the
  * specified error message if so.
  *
  * \param status  The ICU status to check.
  * \param message The message to pass to the UnicodeException on failure.
  */
void check_status(UErrorCode status, const char *message)
{
	if (U_FAILURE(status))
		throw uni::UnicodeException(message);
}

template<typename T>
void check_bogus(const T &object, const char *message)
{
	if (object.isBogus())
		throw uni::UnicodeException(message);
}

namespace uni
{
	template<typename T>
	void ICUDeleter<T>::operator()(T *to_delete)
	{
		delete to_delete;
	}

	template<>
	void ICUDeleter<UText>::operator()(UText *to_delete)
	{
		utext_close(to_delete);
	}

	std::string get_code_name(Code code)
	{
		UErrorCode status = U_ZERO_ERROR;

		// We have to call the u_charName() function twice: once to get the
		// length of the string, and a second time to actually store the string.

		// Add one to the size to include space for the null terminator.
		size_t size = 1 + u_charName(code, U_UNICODE_CHAR_NAME, nullptr, 0, &status);

		// Get the codepoint name itself. The null terminator is already set
		// because new will initialize to zero.
		char *buffer = new char[size];
		u_charName(code, U_UNICODE_CHAR_NAME, buffer, size, &status);

		std::string ret = buffer;
		delete[] buffer;

		if (U_FAILURE(status) || ret.empty())
			return "UNKNOWN";

		return ret;
	}

	const char *get_char_type(Code code)
	{
		UCharCategory type = (UCharCategory)u_charType(code);

		switch (type) {
		case U_UPPERCASE_LETTER:
			return "Lu";
		case U_LOWERCASE_LETTER:
			return "Ll";
		case U_TITLECASE_LETTER:
			return "Lt";
		case U_MODIFIER_LETTER:
			return "Lm";
		case U_OTHER_LETTER:
			return "Lo";
		case U_NON_SPACING_MARK:
			return "Mn";
		case U_ENCLOSING_MARK:
			return "Me";
		case U_COMBINING_SPACING_MARK:
			return "Mc";
		case U_DECIMAL_DIGIT_NUMBER:
			return "Nd";
		case U_LETTER_NUMBER:
			return "Nl";
		case U_OTHER_NUMBER:
			return "No";
		case U_SPACE_SEPARATOR:
			return "Zs";
		case U_LINE_SEPARATOR:
			return "Zl";
		case U_PARAGRAPH_SEPARATOR:
			return "Zp";
		case U_CONTROL_CHAR:
			return "Cc";
		case U_FORMAT_CHAR:
			return "Cf";
		case U_PRIVATE_USE_CHAR:
			return "Co";
		case U_SURROGATE:
			return "Cs";
		case U_DASH_PUNCTUATION:
			return "Pd";
		case U_START_PUNCTUATION:
			return "Ps";
		case U_END_PUNCTUATION:
			return "Pe";
		case U_CONNECTOR_PUNCTUATION:
			return "Pc";
		case U_OTHER_PUNCTUATION:
			return "Po";
		case U_MATH_SYMBOL:
			return "Sm";
		case U_CURRENCY_SYMBOL:
			return "Sc";
		case U_MODIFIER_SYMBOL:
			return "Sk";
		case U_OTHER_SYMBOL:
			return "So";
		case U_INITIAL_PUNCTUATION:
			return "Pi";
		case U_FINAL_PUNCTUATION:
			return "Pf";
		}

		// Anything else is the unknown General Category.
		return "Cn";
	}

	UTF8It::UTF8It(const char *str) :
		m_index(0)
	{
		UErrorCode status = U_ZERO_ERROR;
		m_iterator.reset(utext_openUTF8(nullptr, str, -1, &status));
		check_status(status, "Could not create UTF-8 codepoint iterator");
	}

	UTF8It::UTF8It(const char *str, size_t size) :
		m_index(0)
	{
		UErrorCode status = U_ZERO_ERROR;
		m_iterator.reset(utext_openUTF8(nullptr, str, size, &status));
		check_status(status, "Could not create UTF-8 codepoint iterator");
	}

	UTF8It::UTF8It(const UTF8It &other) :
		m_index(other.m_index)
	{
		UErrorCode status = U_ZERO_ERROR;
		m_iterator.reset(utext_clone(
				nullptr, other.m_iterator.get(), false, false, &status));
		check_status(status, "Could not copy UTF-8 codepoint iterator");
	}

	Code UTF8It::getCode()
	{
		Code result = utext_char32At(m_iterator.get(), m_index);
		m_index = utext_getNativeIndex(m_iterator.get());

		return result;
	}

	Code UTF8It::nextCode()
	{
		Code result = utext_next32From(m_iterator.get(), m_index);
		m_index = utext_getNativeIndex(m_iterator.get());

		return result;
	}

	Code UTF8It::prevCode()
	{
		Code result = utext_previous32From(m_iterator.get(), m_index);
		m_index = utext_getNativeIndex(m_iterator.get());

		return result;
	}

	size_t UTF8It::getEndIndex() const
	{
		utext_nativeLength(m_iterator.get());
	}

	bool operator==(const UTF8It &left, const UTF8It &right)
	{
		/* We don't use UText's native string position for reasons described in
		 * the documentation for m_index. Hence, to make utext_equals() not take
		 * the iterator position into account, set both to zero.
		 */
		utext_setNativeIndex(left.m_iterator.get(), 0);
		utext_setNativeIndex(right.m_iterator.get(), 0);

		return utext_equals(left.m_iterator.get(), right.m_iterator.get()) &&
				left.m_index == right.m_index;
	}

	bool code_to_utf8(Code code, char str[5])
	{
		memset(str, '\0', 5);

		// U8_APPEND() is _very_ particular about its arguments; they must be
		// the exact type that is expects, and some are required to be lvalues.
		uint8_t *ustr = (uint8_t *)str;
		UBool error = false;
		int32_t i = 0;

		U8_APPEND(ustr, i, 5, code, error);

		return error;
	}

	void Locale::setNormalized(const icu::Locale &locale)
	{
		check_bogus(locale, "Invalid locale");

		// We can normalize the locale by extracting just the parts we want
		// and then passing them back in again.
		UErrorCode status = U_ZERO_ERROR;

		std::string locale_str = std::string(locale.getLanguage()) + "_" +
				locale.getScript() + "_" + locale.getCountry();
		m_locale = icu::Locale::createFromName(locale_str.c_str());

		check_status(status, "Unable to build locale");
		check_bogus(locale, "Invalid locale");
	}

	StrPositioner::StrPositioner(std::shared_ptr<const Str> str) :
		m_str(str)
	{
		m_str->registerPositioner(this);
	}

	StrPositioner::StrPositioner(const StrPositioner &other) :
		m_str(other.m_str)
	{
		m_str->registerPositioner(this);
	}

	StrPositioner &StrPositioner::operator=(const StrPositioner &other)
	{
		if (this != &other) {
			m_str = other.m_str;
			m_str->registerPositioner(this);
		}
		return *this;
	}

	StrPositioner::~StrPositioner()
	{
		m_str->unregisterPositioner(this);
	}

	Code StrIt::getCode()
	{
		UText *breaker = m_str->getCodeBreaker();

		Code result = utext_char32At(breaker, m_index);
		m_index = utext_getNativeIndex(breaker);

		return result;
	}

	Code StrIt::nextCode()
	{
		UText *breaker = m_str->getCodeBreaker();

		Code result = utext_next32From(breaker, m_index);
		m_index = utext_getNativeIndex(breaker);

		return result;
	}

	Code StrIt::prevCode()
	{
		UText *breaker = m_str->getCodeBreaker();

		Code result = utext_previous32From(breaker, m_index);
		m_index = utext_getNativeIndex(breaker);

		return result;
	}

	void StrIt::nextChar()
	{
		m_index = m_str->getCharBreaker()->following(m_index);
		correctNext();
	}

	void StrIt::prevChar()
	{
		m_index = m_str->getCharBreaker()->preceding(m_index);
		correctPrev();
	}

	void StrIt::nextWord()
	{
		m_index = m_str->getWordBreaker()->following(m_index);
		correctNext();
	}

	void StrIt::prevWord()
	{
		m_index = m_str->getWordBreaker()->preceding(m_index);
		correctPrev();
	}

	bool StrIt::nextBreak()
	{
		icu::BreakIterator *breaker = m_str->getLineBreaker();
		m_index = breaker->following(m_index);
		correctNext();
		return getLineBreakType(breaker);
	}

	bool StrIt::prevBreak()
	{
		icu::BreakIterator *breaker = m_str->getLineBreaker();
		breaker->preceding(m_index);
		correctPrev();
		return getLineBreakType(breaker);
	}

	void StrIt::nextLine()
	{
		// Simply loop until we find a hard break or hit the end.
		while (!isAtEnd() && !nextBreak());
	}

	void StrIt::prevLine()
	{
		// Simply loop until we find a hard break or hit the start.
		while (!isAtStart() && !prevBreak());
	}

	bool StrIt::isAtStart() const
	{
		return m_index == 0;
	}

	bool StrIt::isAtEnd() const
	{
		return m_index == m_str->size();
	}

	void StrIt::toStart()
	{
		m_index = 0;
	}

	void StrIt::toEnd()
	{
		m_index = m_str->size();
	}

	void StrIt::correctNext()
	{
		if (m_index == (size_t)icu::BreakIterator::DONE)
			toEnd();
	}

	void StrIt::correctPrev()
	{
		if (m_index == (size_t)icu::BreakIterator::DONE)
			toStart();
	}

	bool StrIt::getLineBreakType(icu::BreakIterator *breaker) const
	{
		Code type = breaker->getRuleStatus();
		if (type >= UBRK_LINE_HARD && type < UBRK_LINE_HARD_LIMIT)
			return true;
		return false;
	}

	Str::Str(const Locale &locale) :
		m_locale(locale)
	{
		check_bogus(m_string, "Unable to create string");
		regenerate();
	}

	Str::Str(const char *str, const Locale &locale) :
		m_string(icu::UnicodeString::fromUTF8(str)),
		m_locale(locale)
	{
		check_bogus(m_string, "Unable to create string");
		regenerate();
	}

	Str::Str(const std::string &str, const Locale &locale) :
		m_string(icu::UnicodeString::fromUTF8(str)),
		m_locale(locale)
	{
		check_bogus(m_string, "Unable to create string");
		regenerate();
	}

	Str &Str::operator=(const Str &other)
	{
		if (this != &other) {
			m_string = other.m_string;
			m_locale = other.m_locale;

			regenerate();
		}
		return *this;
	}

	std::shared_ptr<Str> Str::sub(const StrIt &begin, const StrIt &end) const
	{
		size_t begin_i = begin.getIndex();
		size_t end_i = end.getIndex();

		std::shared_ptr<Str> str = Str::create();
		m_string.extract(begin_i, end_i - begin_i, str->m_string);

		return str;
	}

	void Str::append(std::shared_ptr<Str> str)
	{
		size_t pos_i = str->size();
		m_string.append(str->m_string);
		updateArea(pos_i, pos_i, size());
	}

	void Str::insert(const StrIt &pos, std::shared_ptr<Str> str)
	{
		size_t pos_i = pos.getIndex();

		m_string.insert(pos_i, str->m_string);
		updateArea(pos_i, pos_i, pos_i + str->size());
	}

	void Str::replace(const StrIt &begin, const StrIt &end, std::shared_ptr<Str> str)
	{
		size_t begin_i = begin.getIndex();
		size_t end_i = end.getIndex();

		m_string.replace(begin_i, end_i - begin_i, str->m_string);
		updateArea(begin_i, end_i, begin_i + str->size());
	}

	void Str::remove(const StrIt &begin, const StrIt &end)
	{
		size_t begin_i = begin.getIndex();
		size_t end_i = end.getIndex();

		m_string.remove(begin_i, end_i);
		updateArea(begin_i, end_i, begin_i);
	}

	void Str::clear()
	{
		m_string.remove();
		updateEntire();
	}

	void Str::trim()
	{
		m_string.trim();
		updateEntire();
	}

	void Str::toLower()
	{
		m_string.toLower(m_locale.getInternal());
		updateEntire();
	}

	void Str::toUpper()
	{
		m_string.toUpper(m_locale.getInternal());
		updateEntire();
	}

	void Str::toTitle(bool first_only)
	{
		m_string.toTitle(getWordBreaker(), m_locale.getInternal(),
				first_only ? U_TITLECASE_NO_LOWERCASE : 0);
		updateEntire();
	}

	void Str::normalize(bool compose)
	{
		UErrorCode status = U_ZERO_ERROR;

		const icu::Normalizer2 *normalizer;
		if (compose) {
			normalizer = icu::Normalizer2::getNFCInstance(status);
		} else {
			normalizer = icu::Normalizer2::getNFDInstance(status);
		}
		m_string = normalizer->normalize(m_string, status);

		check_status(status, "Could not normalize string");

		updateEntire();
	}

	void Str::setLocale(const Locale &locale)
	{
		m_locale = locale;

		// Since everything is dependent on locale, update everything.
		regenerate();
		updateEntire();
	}

	void Str::regenerate()
	{
		m_code_breaker = nullptr;
		m_char_breaker = nullptr;
		m_word_breaker = nullptr;
		m_line_breaker = nullptr;
	}

	void Str::updateBreakers()
	{
		UErrorCode status = U_ZERO_ERROR;

		m_code_breaker.reset(utext_openUnicodeString(
				m_code_breaker.get(), &m_string, &status));

		check_status(status, "Could not create codepoint iterator");

		if (m_char_breaker != nullptr)
			m_char_breaker->setText(m_string);
		if (m_word_breaker != nullptr)
			m_word_breaker->setText(m_string);
		if (m_line_breaker != nullptr)
			m_line_breaker->setText(m_string);
	}

	void Str::updatePositioners()
	{
		for (StrPositioner *poser : m_positioners)
			poser->updatePositioner();
	}

	void Str::updateArea(size_t start, size_t old_end, size_t new_end)
	{
		// Rebuild or change break iterators since the string has changed.
		updateBreakers();

		// Move all StrIts affected by the change so they still point to the
		// proper positions.
		for (StrPositioner *poser : m_positioners) {
			StrIt *it = dynamic_cast<StrIt *>(poser);

			// If this isn't a StrIt, do nothing.
			if (it == nullptr)
				continue;

			size_t index = it->getIndex();

			if (index > old_end) {
				// If we're past the changed area, hold our position.
				it->setIndex(index - old_end + new_end);
			} else if (index > start) {
				// If we're inside the changed area, move to the end of the new area.
				it->setIndex(new_end);
			}
		}

		// Then, now that the StrIts are updated, update all the positioners.
		updatePositioners();
	}

	void Str::updateEntire()
	{
		updateBreakers();

		for (StrPositioner *poser : m_positioners) {
			StrIt *it = dynamic_cast<StrIt *>(poser);

			// If this is a StrIt, set its position to the end of the string.
			if (it != nullptr)
				it->setIndex(size());
		}

		updatePositioners();
	}

	icu::BreakIterator *Str::getBreaker(ICUUniquePtr<icu::BreakIterator> &breaker,
			BreakerCreator creator) const
	{
		if (breaker.get() == nullptr) {
			UErrorCode status = U_ZERO_ERROR;
			breaker.reset(creator(m_locale.getInternal(), status));
			check_status(status, "Could not create break iterator");

			breaker->setText(m_string);
		}

		return breaker.get();
	}

	icu::BreakIterator *Str::getCharBreaker() const
	{
		return getBreaker(m_char_breaker, &icu::BreakIterator::createCharacterInstance);
	}

	icu::BreakIterator *Str::getWordBreaker() const
	{
		return getBreaker(m_char_breaker, &icu::BreakIterator::createWordInstance);
	}

	icu::BreakIterator *Str::getLineBreaker() const
	{
		return getBreaker(m_char_breaker, &icu::BreakIterator::createLineInstance);
	}

	/** Configures the level of collation for a normal or string searching
	  * collator, taking into account IGNORE_CASE and IGNORE_DIACRITICS.
	  *
	  * \param collator The collator to configure the strength for.
	  * \param options  The set of options, either a Collator::Options or a
	  *                 Searcher::Options. Only IGNORE_CASE and
	  *                 IGNORE_DIACRITICS are considered.
	  * \param status   The ICU status code reported by called ICU functions.
	  */
	void configure_basic_collator(icu::Collator *collator, Collator::Options options,
			UErrorCode &status)
	{
		UColAttributeValue strength;
		UColAttributeValue case_level = UCOL_OFF;

		// Since ICU's comparison levels are mutually exclusive, to ignore
		// diacritics while keeping casing distinct, we have to choose the
		// primary level and turn on the extra case level.
		if (options & Collator::IGNORE_DIACRITICS) {
			strength = UCOL_PRIMARY;
			if (!(options & Collator::IGNORE_CASE))
				case_level = UCOL_ON;
		} else {
			strength = (options & Collator::IGNORE_CASE) ? UCOL_SECONDARY : UCOL_TERTIARY;
		}

		collator->setAttribute(UCOL_STRENGTH, strength, status);
		collator->setAttribute(UCOL_CASE_LEVEL, case_level, status);
	}

	Collator::Collator(const Locale &locale, Options options) :
		m_locale(locale),
		m_options(options)
	{
		regenerate();
	}

	int Collator::collate(std::shared_ptr<Str> left, std::shared_ptr<Str> right) const
	{
		UErrorCode status = U_ZERO_ERROR;

		UCollationResult result = m_collator->compare(left->getInternal(),
				right->getInternal(), status);

		check_status(status, "Unable to perform collation");

		if (result == UCOL_LESS) {
			return -1;
		} else if (result == UCOL_EQUAL) {
			return 0;
		} else {
			return 1;
		}
	}

	void Collator::regenerate()
	{
		UErrorCode status = U_ZERO_ERROR;
		m_collator.reset(icu::Collator::createInstance(m_locale.getInternal(), status));
		check_status(status, "Could not configure collator");

		configure();
	}

	void Collator::configure()
	{
		UErrorCode status = U_ZERO_ERROR;

		configure_basic_collator(m_collator.get(), m_options, status);

		// Set which casing should take precedence or whether the default for
		// this locale should be used.
		UColAttributeValue case_first = UCOL_OFF;
		if (m_options & Collator::UPPERCASE_FIRST)
			case_first = UCOL_UPPER_FIRST;
		else if (m_options & Collator::LOWERCASE_FIRST)
			case_first = UCOL_LOWER_FIRST;
		m_collator->setAttribute(UCOL_CASE_FIRST, case_first, status);

		check_status(status, "Could not configure collator");
	}

	Searcher::Searcher(std::shared_ptr<const Str> str, std::shared_ptr<Str> pattern,
			Options options) :
		StrPositioner(str),
		m_pattern(uni::Str::create(pattern)),
		m_options(options),
		m_start(m_str->itStart()),
		m_end(m_str->itStart()),
		m_needs_regen(true)
	{}

	bool Searcher::next()
	{
		checkRegenerate();

		// If there's no searcher (meaning either the parent string or the
		// pattern is empty), there is no match. Set both iterators to the end.
		if (m_searcher == nullptr) {
			toEnd();
			return false;
		}

		UErrorCode status = U_ZERO_ERROR;
		size_t index = m_searcher->following(m_start.getIndex(), status);
		check_status(status, "Unable to perform string search");

		// If we haven't hit the end, a match was found; return true.
		if (index != (size_t)USEARCH_DONE)
			return true;

		if (m_options & WRAP_AROUND && m_start != m_str->itStart()) {
			// If we should wrap around, try the search again from the beginning.
			// However, if we're already at the start, it means there is no match
			// anywhere, so don't go into an infinite loop of searching.
			return following(m_str->itStart());
		} else {
			// Otherwise, there's no match: set both iterators to the end.
			toEnd();
			return false;
		}
	}

	bool Searcher::prev()
	{
		// Same as next(), but everything in the opposite direction. Trying to
		// generalize it leads to a disgusting mess of pointer-to-member functions.
		checkRegenerate();

		if (m_searcher == nullptr) {
			toStart();
			return false;
		}

		UErrorCode status = U_ZERO_ERROR;
		size_t index = m_searcher->preceding(m_start.getIndex(), status);
		check_status(status, "Unable to perform string search");

		if (index != (size_t)USEARCH_DONE)
			return true;

		if (m_options & WRAP_AROUND && m_start != m_str->itEnd()) {
			return preceding(m_str->itEnd());
		} else {
			toStart();
			return false;
		}
	}

	void Searcher::regenerate()
	{
		if (m_str->isEmpty() || m_pattern->isEmpty())
			m_searcher = nullptr;

		UErrorCode status = U_ZERO_ERROR;

		icu::BreakIterator *breaker = nullptr;
		if (m_options & WHOLE_WORDS_ONLY)
			breaker = m_str->getWordBreaker();

		m_searcher.reset(new icu::StringSearch(m_pattern->getInternal(),
				m_str->getInternal(), m_str->getLocale().getInternal(),
				breaker, status));

		configure_basic_collator(m_searcher->getCollator(),
				(Collator::Options)m_options, status);

		m_searcher->setAttribute(USEARCH_OVERLAP,
				(m_options & OVERLAP) ? USEARCH_ON : USEARCH_OFF, status);

		check_status(status, "Could not configure string searcher");
	}
}

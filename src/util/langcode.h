#pragma once

#include "util/string.h"
#include <algorithm>
#include <vector>

template<typename T>
static std::basic_string_view<T> find_tr_language(const std::basic_string_view<T> &code)
{
	// Strip encoding (".UTF-8") information if it is present as it is not relevant
	auto pos = code.find(T('.'));
	if (pos != code.npos)
		return code.substr(0, pos);

	return code;
}

template<typename T>
static std::basic_string_view<T> find_tr_language(const std::basic_string<T> &code)
{
	return find_tr_language(std::basic_string_view<T>(code));
}

template<typename T>
inline std::vector<std::basic_string<T>> split_language_list(const std::basic_string<T> &str)
{
	auto list = str_split(str, T(':'));
	for (auto &i: list)
		i = find_tr_language(i);
	auto newend = std::remove(list.begin(), list.end(), std::basic_string<T>());
	return std::vector<std::basic_string<T>>(list.begin(), newend);
}

template<typename T>
inline std::basic_string<T> language_list_to_string(const std::vector<std::basic_string<T>> &list)
{
	constexpr T delimiter(':');
	return str_join(list, std::basic_string_view<T>(&delimiter, 1));
}

std::vector<std::wstring> parse_language_list(const std::vector<std::wstring> &lang);

inline std::vector<std::wstring> parse_language_list(const std::wstring &lang)
{
	return parse_language_list(split_language_list(lang));
}

inline const std::string get_tr_language(const std::string &lang)
{
	return wide_to_utf8(language_list_to_string(parse_language_list(utf8_to_wide(lang))));
}

template<typename T>
inline const std::basic_string<T> get_primary_language(const std::basic_string<T> &lang)
{
	auto delimiter_pos = lang.find(':');
	if (delimiter_pos == std::basic_string<T>::npos)
		return lang;
	return lang.substr(0, delimiter_pos);
}

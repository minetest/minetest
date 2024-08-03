#pragma once

#include "util/string.h"
#include <vector>

template<typename T>
inline std::vector<std::basic_string<T>> split_language_list(const std::basic_string<T> &list)
{
	return str_split(list, T(':'));
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

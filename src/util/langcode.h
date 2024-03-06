#pragma once

#include "util/string.h"
#include <vector>

template<typename T>
inline std::vector<std::basic_string<T>> parse_language_list(const std::basic_string<T> &list)
{
	return str_split(list, T(':'));
}

inline std::string language_list_to_string(const std::vector<std::string> &list)
{
	return str_join(list, ":");
}

std::vector<std::string> get_tr_language(const std::vector<std::string> &lang);

inline const std::string get_tr_language(const std::string &lang)
{
	return language_list_to_string(get_tr_language(parse_language_list(lang)));
}

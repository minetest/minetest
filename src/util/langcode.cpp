#include "langcode.h"
#include <unordered_set>
#include <unordered_map>

static std::wstring_view find_tr_language(const std::wstring_view &code)
{
	// Strip encoding (".UTF-8") if they are present as we currently don't use them
	auto pos = code.find('.');
	if (pos != code.npos)
		return code.substr(0, pos);

	return code;
}

struct ParserCache {
	using string = std::wstring;
	using string_view = std::wstring_view;
	std::unordered_set<string_view> manual;
	std::unordered_map<string_view, string_view> prev;
	std::unordered_map<string_view, string_view> next;

	string_view variant_of(const string_view &str)
	{
		auto pos = str.find_last_of(L"_@");
		if (pos != str.npos)
			return str.substr(0, pos);
		return str.substr(0, 0);
	}

	void queue(const string_view &str, const bool is_manual)
	{
		if (str.empty())
			return;

		if (is_manual) {
			manual.emplace(str);
			next.erase(prev[str]);
			prev.erase(str);
		}

		auto variant = variant_of(str);
		if (!variant.empty() && manual.find(variant) == manual.end()) {
			next.erase(prev[variant]);
			next[str] = variant;
			prev[variant] = str;
			queue(variant, false);
		}
	}

	void add_to_list(std::vector<string> &list, const string_view &str)
	{
		list.emplace_back(str);
		if (next.find(str) != next.end())
			add_to_list(list, next[str]);
	}
};

std::vector<std::wstring> parse_language_list(const std::vector<std::wstring> &languages)
{
	ParserCache state;
	for (const auto &lang: languages)
		state.queue(find_tr_language(lang), true);
	std::vector<std::wstring> newlist;
	for (const auto &lang: languages)
		state.add_to_list(newlist, lang);
	return newlist;
}

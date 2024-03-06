#include "langcode.h"
#include "util/languagemap.h"
#include <set>
#include <utility>

// We don't need the lanugae name here, only the language code
#define ENTRY(code, name) code
static const std::set<std::string> language_codes { LANGUAGE_MAP(ENTRY) };
#undef ENTRY

// We are not using C++20 so set::contains is not available
static inline bool contains(const std::set<std::string> &set, const std::string &entry)
{
	return set.find(entry) != set.end();
}

static std::string find_tr_language(const std::string &code)
{
	if (contains(language_codes, code))
		return code;

	// Strip encoding (".UTF-8") and variant ("@...") if they are present as we currently don't use them
	auto pos = code.find_first_of(".@");
	if (pos != code.npos)
		return find_tr_language(code.substr(0, pos));

	// Strip regional information if they are present
	pos = code.find('_');
	if (pos != code.npos)
		return find_tr_language(code.substr(0, pos));

	return "";
}

std::vector<std::string> get_tr_language(const std::vector<std::string> &languages)
{
	std::vector<std::string> list;
	for (auto language: languages) {
		std::string tr_lang = find_tr_language(language);
		if (!tr_lang.empty())
			list.push_back(std::move(tr_lang));
	}
	return list;
}

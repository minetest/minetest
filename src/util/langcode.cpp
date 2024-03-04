#include "langcode.h"
#include <utility>

static std::string find_tr_language(const std::string &code)
{
	// Strip encoding (".UTF-8") and variant ("@...") if they are present as we currently don't use them
	auto pos = code.find_first_of(".@");
	if (pos != code.npos)
		return code.substr(0, pos);

	return code;
}

std::vector<std::string> get_tr_language(const std::vector<std::string> &languages)
{
	std::vector<std::string> list;
	for (const auto &language: languages) {
		std::string tr_lang = find_tr_language(language);
		if (!tr_lang.empty())
			list.push_back(std::move(tr_lang));
	}
	return list;
}

#ifndef SERVER
std::string get_client_language_code() {
#if USE_GETTEXT
	const char *lang = getenv("LANGUAGE");
	if (!lang)
		return "";
	return get_tr_language(lang);
#else
	return "";
#endif
}
#endif

#ifndef GETTEXT_HEADER
#include "config.h" // for USE_GETTEXT
#include <iostream>

#if USE_GETTEXT
#include <libintl.h>
#else
#define gettext(String) String
#endif

#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

inline void init_gettext(const char *path) {
#if USE_GETTEXT
	// don't do this if MSVC compiler is used, it gives an assertion fail
	#ifndef _MSC_VER
		setlocale(LC_MESSAGES, "");
	#endif
	bindtextdomain(PROJECT_NAME, path);
	textdomain(PROJECT_NAME);
#endif
}

inline wchar_t* chartowchar_t(const char *str)
{
	size_t l = strlen(str)+1;
	wchar_t* nstr = new wchar_t[l];
	mbstowcs(nstr, str, l);
	return nstr;
}

inline wchar_t* wgettext(const char *str)
{
	return chartowchar_t(gettext(str));
}

inline void changeCtype(const char *l)
{
	char *ret = NULL;
	ret = setlocale(LC_CTYPE, l);
	if(ret == NULL)
		std::cout<<"locale could not be set"<<std::endl;
	else
		std::cout<<"locale has been set to:"<<ret<<std::endl;
}
#define GETTEXT_HEADER
#endif

#if defined(GETTEXT_FOUND) && defined(USE_GETTEXT)
#include <libintl.h>
#else
#define gettext(String) String
#endif

#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

inline void init_gettext(const char *path) {
#if USE_GETTEXT
	setlocale(LC_MESSAGES, "");
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

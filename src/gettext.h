#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

inline wchar_t* chartowchar_t(char *str)
{
	size_t l = strlen(str)+1;
	wchar_t* nstr = new wchar_t[l];
	mbstowcs(nstr, str, l);
	return nstr;
}

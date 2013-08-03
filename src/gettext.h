#ifndef GETTEXT_HEADER
#include "config.h" // for USE_GETTEXT
#include "log.h"

#if USE_GETTEXT
#include <libintl.h>
#else
#define gettext(String) String
#endif

#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>
#endif

inline void init_gettext(const char *path) {
#if USE_GETTEXT
	// don't do this if MSVC compiler is used, it gives an assertion fail
	#ifndef _MSC_VER
		setlocale(LC_MESSAGES, "");
	#endif
	bindtextdomain(PROJECT_NAME, path);
	textdomain(PROJECT_NAME);
#if defined(_WIN32)
	// As linux is successfully switched to UTF-8 completely at about year 2005
	// Windows still uses obsolete codepage based locales because you
	// cannot recompile closed-source applications

	// Set character encoding for Win32
	char *tdomain = textdomain( (char *) NULL );
	if( tdomain == NULL )
	{
		fprintf( stderr, "warning: domainname parameter is the null pointer, default domain is not set\n" );
		tdomain = (char *) "messages";
	}
	/*char *codeset = */bind_textdomain_codeset( tdomain, "UTF-8" );
	//fprintf( stdout, "%s: debug: domainname = %s; codeset = %s\n", argv[0], tdomain, codeset );
#endif // defined(_WIN32)
#endif
}

inline wchar_t* chartowchar_t(const char *str)
{
	wchar_t* nstr = 0;
#if defined(_WIN32)
	int nResult = MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) str, -1, 0, 0 );
	if( nResult == 0 )
	{
		fprintf( stderr, "error: MultiByteToWideChar returned null\n" );
	}
	else
	{
		nstr = new wchar_t[nResult];
		MultiByteToWideChar( CP_UTF8, 0, (LPCSTR) str, -1, (WCHAR *) nstr, nResult );
	}
#else
	size_t l = strlen(str)+1;
	nstr = new wchar_t[l];
	mbstowcs(nstr, str, l);
#endif

	return nstr;
}

inline wchar_t* wgettext(const char *str)
{
	return chartowchar_t(gettext(str));
}

inline void changeCtype(const char *l)
{
	/*char *ret = NULL;
	ret = setlocale(LC_CTYPE, l);
	if(ret == NULL)
		infostream<<"locale could not be set"<<std::endl;
	else
		infostream<<"locale has been set to:"<<ret<<std::endl;*/
}

inline std::wstring wstrgettext(std::string text) {
	wchar_t* wlabel = wgettext(text.c_str());
	std::wstring out = (std::wstring)wlabel;
	delete[] wlabel;
	return out;
}
#define GETTEXT_HEADER
#endif

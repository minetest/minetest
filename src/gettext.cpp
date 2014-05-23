/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <string>
#include <string.h>
#include <iostream>
#include <stdlib.h>
#include "gettext.h"
#include "util/string.h"

#if USE_GETTEXT && defined(_MSC_VER)
#include <WinNls.h>
#include <map>
#include <direct.h>
#include "filesys.h"

#define setlocale(category,localename) \
	setlocale(category,MSVC_LocaleLookup(localename))

static std::map<std::wstring,std::wstring> glb_supported_locales;

/******************************************************************************/
BOOL CALLBACK UpdateLocaleCallback(LPTSTR pStr)
{
	char* endptr = 0;
	int LOCALEID = strtol(pStr,&endptr,16);

	wchar_t buffer[LOCALE_NAME_MAX_LENGTH];
	memset(buffer,0,sizeof(buffer));
	if (GetLocaleInfoW(
		LOCALEID,
		LOCALE_SISO639LANGNAME,
		buffer,
		LOCALE_NAME_MAX_LENGTH)) {

		std::wstring name = buffer;

		memset(buffer,0,sizeof(buffer));
		GetLocaleInfoW(
		LOCALEID,
		LOCALE_SISO3166CTRYNAME,
		buffer,
		LOCALE_NAME_MAX_LENGTH);

		std::wstring country = buffer;

		memset(buffer,0,sizeof(buffer));
		GetLocaleInfoW(
		LOCALEID,
		LOCALE_SENGLISHLANGUAGENAME,
		buffer,
		LOCALE_NAME_MAX_LENGTH);

		std::wstring languagename = buffer;

		/* set both short and long variant */
		glb_supported_locales[name] = languagename;
		glb_supported_locales[name + L"_" + country] = languagename;
	}
	return true;
}

/******************************************************************************/
const char* MSVC_LocaleLookup(const char* raw_shortname) {

	/* NULL is used to read locale only so we need to return it too */
	if (raw_shortname == NULL) return NULL;

	std::string shortname(raw_shortname);
	if (shortname == "C") return "C";
	if (shortname == "") return "";

	static std::string last_raw_value = "";
	static std::string last_full_name = "";
	static bool first_use = true;

	if (last_raw_value == shortname) {
		return last_full_name.c_str();
	}

	if (first_use) {
		EnumSystemLocalesA(UpdateLocaleCallback,LCID_SUPPORTED | LCID_ALTERNATE_SORTS);
		first_use = false;
	}

	last_raw_value = shortname;

	if (glb_supported_locales.find(narrow_to_wide(shortname)) != glb_supported_locales.end()) {
		last_full_name = wide_to_narrow(glb_supported_locales[narrow_to_wide(shortname)]);
		return last_full_name.c_str();
	}

	/* empty string is system default */
	errorstream << "MSVC_LocaleLookup: unsupported locale: \"" << shortname
				<< "\" switching to system default!" << std::endl;
	return "";
}

#endif

/******************************************************************************/
#ifdef _MSC_VER
void init_gettext(const char *path,std::string configured_language,int argc, char** argv) {
#else
void init_gettext(const char *path,std::string configured_language) {
#endif
#if USE_GETTEXT
	/** first try to set user override environment **/
	if (configured_language.length() != 0) {
#ifndef _WIN32
		/* add user specified locale to environment */
		setenv("LANGUAGE", configured_language.c_str(), 1);

		/* reload locale with changed environment */
		setlocale(LC_ALL, "");
#elif defined(_MSC_VER)
		std::string current_language_var("");
		if (getenv("LANGUAGE") != 0) {
			current_language_var = std::string(getenv("LANGUAGE"));
		}

		char *lang_str = (char*)calloc(10 + configured_language.length(), sizeof(char));
		strcat(lang_str, "LANGUAGE=");
		strcat(lang_str, configured_language.c_str());
		putenv(lang_str);

		SetEnvironmentVariableA("LANGUAGE",configured_language.c_str());

#ifndef SERVER
		//very very dirty workaround to force gettext to see the right environment
		if (current_language_var != configured_language) {
			STARTUPINFO startupinfo;
			PROCESS_INFORMATION processinfo;
			memset(&startupinfo,0,sizeof(startupinfo));
			memset(&processinfo,0,sizeof(processinfo));
			errorstream << "MSVC localization workaround active restating minetest in new environment!" << std::endl;

			std::string parameters = "";

			for (unsigned int i=1;i < argc; i++) {
				if (parameters != "") {
					parameters += " ";
				}
				parameters += argv[i];
			}

			const char* ptr_parameters = 0;

			if (parameters != "") {
				ptr_parameters = parameters.c_str();
			}
			
			/** users may start by short name in commandline without extention **/
			std::string appname = argv[0];
			if (appname.substr(appname.length() -4) != ".exe") {
				appname += ".exe";
			}

			if (!CreateProcess(appname.c_str(),
								(char*) ptr_parameters,
								NULL,
								NULL,
								false,
								DETACHED_PROCESS | CREATE_UNICODE_ENVIRONMENT,
								NULL,
								NULL,
								&startupinfo,
								&processinfo)) {
				char buffer[1024];		
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
					buffer,
					sizeof(buffer)-1,
					NULL);
				errorstream << "*******************************************************" << std::endl;
				errorstream << "CMD: " << appname << std::endl;
				errorstream << "Failed to restart with current locale: " << std::endl;
				errorstream << buffer;
				errorstream << "Expect language to be broken!" << std::endl;
				errorstream << "*******************************************************" << std::endl;
			}
			else {
				exit(0);
			}
#else
			errorstream << "*******************************************************" << std::endl;
			errorstream << "Can't apply locale workaround for server!" << std::endl;
			errorstream << "Expect language to be broken!" << std::endl;
			errorstream << "*******************************************************" << std::endl;

#endif
		}

		setlocale(LC_ALL,configured_language.c_str());
#else // Mingw
		char *lang_str = (char*)calloc(10 + configured_language.length(), sizeof(char));
		strcat(lang_str, "LANGUAGE=");
		strcat(lang_str, configured_language.c_str());
		putenv(lang_str);

		setlocale(LC_ALL, "");
#endif // ifndef _WIN32
	}
	else {
		 /* set current system default locale */
		setlocale(LC_ALL, "");
	}

#if defined(_WIN32)
	if (getenv("LANGUAGE") != 0) {
		setlocale(LC_ALL, getenv("LANGUAGE"));
	}
#ifdef _MSC_VER
	else if (getenv("LANG") != 0) {
		setlocale(LC_ALL, getenv("LANG"));
	}
#endif
#endif

	bindtextdomain(PROJECT_NAME, path);
	textdomain(PROJECT_NAME);

#if defined(_WIN32)
	// Set character encoding for Win32
	char *tdomain = textdomain( (char *) NULL );
	if( tdomain == NULL )
	{
		errorstream << "Warning: domainname parameter is the null pointer" <<
				", default domain is not set" << std::endl;
		tdomain = (char *) "messages";
	}
	/* char *codeset = */bind_textdomain_codeset( tdomain, "UTF-8" );
	//errorstream << "Gettext debug: domainname = " << tdomain << "; codeset = "<< codeset << std::endl;
#endif // defined(_WIN32)

#else
	/* set current system default locale */
	setlocale(LC_ALL, "");
#endif // if USE_GETTEXT

	/* no matter what locale is used we need number format to be "C" */
	/* to ensure formspec parameters are evaluated correct!          */

	setlocale(LC_NUMERIC,"C");
	infostream << "Message locale is now set to: "
			<< setlocale(LC_ALL,0) << std::endl;
}





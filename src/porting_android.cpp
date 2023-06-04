/*
Minetest
Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef __ANDROID__
#error This file may only be compiled for android!
#endif

#include "util/numeric.h"
#include "porting.h"
#include "porting_android.h"
#include "threading/thread.h"
#include "config.h"
#include "filesys.h"
#include "log.h"

#include <sstream>
#include <exception>
#include <cstdlib>

#ifdef GPROF
#include "prof.h"
#endif

extern int main(int argc, char *argv[]);

void android_main(android_app *app)
{
	int retval = 0;
	porting::app_global = app;

	Thread::setName("Main");

	char *argv[] = {strdup(PROJECT_NAME), strdup("--verbose"), nullptr};
	try {
		main(ARRLEN(argv) - 1, argv);
	} catch (std::exception &e) {
		errorstream << "Uncaught exception in main thread: " << e.what() << std::endl;
		retval = -1;
	} catch (...) {
		errorstream << "Uncaught exception in main thread!" << std::endl;
		retval = -1;
	}
	free(argv[0]);
	free(argv[1]);

	porting::cleanupAndroid();
	infostream << "Shutting down." << std::endl;
	exit(retval);
}

namespace porting {
android_app *app_global;
JNIEnv      *jnienv;
jclass       nativeActivity;

jclass findClass(const std::string &classname)
{
	if (jnienv == nullptr)
		return nullptr;

	jclass nativeactivity = jnienv->FindClass("android/app/NativeActivity");
	jmethodID getClassLoader = jnienv->GetMethodID(
			nativeactivity, "getClassLoader", "()Ljava/lang/ClassLoader;");
	jobject cls = jnienv->CallObjectMethod(
						app_global->activity->clazz, getClassLoader);
	jclass classLoader = jnienv->FindClass("java/lang/ClassLoader");
	jmethodID findClass = jnienv->GetMethodID(classLoader, "loadClass",
					"(Ljava/lang/String;)Ljava/lang/Class;");
	jstring strClassName = jnienv->NewStringUTF(classname.c_str());
	return (jclass) jnienv->CallObjectMethod(cls, findClass, strClassName);
}

void initAndroid()
{
	porting::jnienv = nullptr;
	JavaVM *jvm = app_global->activity->vm;
	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = PROJECT_NAME_C "NativeThread";
	lJavaVMAttachArgs.group = nullptr;

	if (jvm->AttachCurrentThread(&porting::jnienv, &lJavaVMAttachArgs) == JNI_ERR) {
		errorstream << "Failed to attach native thread to jvm" << std::endl;
		exit(-1);
	}

	nativeActivity = findClass("net/minetest/minetest/GameActivity");
	if (nativeActivity == nullptr)
		errorstream <<
			"porting::initAndroid unable to find Java native activity class" <<
			std::endl;

#ifdef GPROF
	// in the start-up code
	__android_log_print(ANDROID_LOG_ERROR, PROJECT_NAME_C,
			"Initializing GPROF profiler");
	monstartup("libMinetest.so");
#endif
}

void cleanupAndroid()
{
#ifdef GPROF
	errorstream << "Shutting down GPROF profiler" << std::endl;
	setenv("CPUPROFILE", (path_user + DIR_DELIM + "gmon.out").c_str(), 1);
	moncleanup();
#endif

	JavaVM *jvm = app_global->activity->vm;
	jvm->DetachCurrentThread();
}

static std::string readJavaString(jstring j_str)
{
	// Get string as a UTF-8 C string
	const char *c_str = jnienv->GetStringUTFChars(j_str, nullptr);
	// Save it
	std::string str(c_str);
	// And free the C string
	jnienv->ReleaseStringUTFChars(j_str, c_str);
	return str;
}

void initializePathsAndroid()
{
	// Set user and share paths
	{
		jmethodID getUserDataPath = jnienv->GetMethodID(nativeActivity,
				"getUserDataPath", "()Ljava/lang/String;");
		FATAL_ERROR_IF(getUserDataPath==nullptr,
				"porting::initializePathsAndroid unable to find Java getUserDataPath method");
		jobject result = jnienv->CallObjectMethod(app_global->activity->clazz, getUserDataPath);
		std::string str = readJavaString((jstring) result);
		path_user = str;
		path_share = str;
		path_locale = str + DIR_DELIM + "locale";
	}

	// Set cache path
	{
		jmethodID getCachePath = jnienv->GetMethodID(nativeActivity,
				"getCachePath", "()Ljava/lang/String;");
		FATAL_ERROR_IF(getCachePath==nullptr,
				"porting::initializePathsAndroid unable to find Java getCachePath method");
		jobject result = jnienv->CallObjectMethod(app_global->activity->clazz, getCachePath);
		path_cache = readJavaString((jstring) result);

		migrateCachePath();
	}
}

void showInputDialog(const std::string &acceptButton, const std::string &hint,
		const std::string &current, int editType)
{
	jmethodID showdialog = jnienv->GetMethodID(nativeActivity, "showDialog",
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");

	FATAL_ERROR_IF(showdialog == nullptr,
		"porting::showInputDialog unable to find Java showDialog method");

	jstring jacceptButton = jnienv->NewStringUTF(acceptButton.c_str());
	jstring jhint         = jnienv->NewStringUTF(hint.c_str());
	jstring jcurrent      = jnienv->NewStringUTF(current.c_str());
	jint    jeditType     = editType;

	jnienv->CallVoidMethod(app_global->activity->clazz, showdialog,
			jacceptButton, jhint, jcurrent, jeditType);
}

void openURIAndroid(const std::string &url)
{
	jmethodID url_open = jnienv->GetMethodID(nativeActivity, "openURI",
		"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
		"porting::openURIAndroid unable to find Java openURI method");

	jstring jurl = jnienv->NewStringUTF(url.c_str());
	jnienv->CallVoidMethod(app_global->activity->clazz, url_open, jurl);
}

void shareFileAndroid(const std::string &path)
{
	jmethodID url_open = jnienv->GetMethodID(nativeActivity, "shareFile",
			"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
			"porting::shareFileAndroid unable to find Java shareFile method");

	jstring jurl = jnienv->NewStringUTF(path.c_str());
	jnienv->CallVoidMethod(app_global->activity->clazz, url_open, jurl);
}

int getInputDialogState()
{
	jmethodID dialogstate = jnienv->GetMethodID(nativeActivity,
			"getDialogState", "()I");

	FATAL_ERROR_IF(dialogstate == nullptr,
		"porting::getInputDialogState unable to find Java getDialogState method");

	return jnienv->CallIntMethod(app_global->activity->clazz, dialogstate);
}

std::string getInputDialogValue()
{
	jmethodID dialogvalue = jnienv->GetMethodID(nativeActivity,
			"getDialogValue", "()Ljava/lang/String;");

	FATAL_ERROR_IF(dialogvalue == nullptr,
		"porting::getInputDialogValue unable to find Java getDialogValue method");

	jobject result = jnienv->CallObjectMethod(app_global->activity->clazz,
			dialogvalue);
	return readJavaString((jstring) result);
}

#ifndef SERVER
float getDisplayDensity()
{
	static bool firstrun = true;
	static float value = 0;

	if (firstrun) {
		jmethodID getDensity = jnienv->GetMethodID(nativeActivity,
				"getDensity", "()F");

		FATAL_ERROR_IF(getDensity == nullptr,
			"porting::getDisplayDensity unable to find Java getDensity method");

		value = jnienv->CallFloatMethod(app_global->activity->clazz, getDensity);
		firstrun = false;
	}

	return value;
}

v2u32 getDisplaySize()
{
	static bool firstrun = true;
	static v2u32 retval;

	if (firstrun) {
		jmethodID getDisplayWidth = jnienv->GetMethodID(nativeActivity,
				"getDisplayWidth", "()I");

		FATAL_ERROR_IF(getDisplayWidth == nullptr,
			"porting::getDisplayWidth unable to find Java getDisplayWidth method");

		retval.X = jnienv->CallIntMethod(app_global->activity->clazz,
				getDisplayWidth);

		jmethodID getDisplayHeight = jnienv->GetMethodID(nativeActivity,
				"getDisplayHeight", "()I");

		FATAL_ERROR_IF(getDisplayHeight == nullptr,
			"porting::getDisplayHeight unable to find Java getDisplayHeight method");

		retval.Y = jnienv->CallIntMethod(app_global->activity->clazz,
				getDisplayHeight);

		firstrun = false;
	}

	return retval;
}

std::string getLanguageAndroid()
{
	jmethodID getLanguage = jnienv->GetMethodID(nativeActivity,
			"getLanguage", "()Ljava/lang/String;");

	FATAL_ERROR_IF(getLanguage == nullptr,
		"porting::getLanguageAndroid unable to find Java getLanguage method");

	jobject result = jnienv->CallObjectMethod(app_global->activity->clazz,
			getLanguage);
	return readJavaString((jstring) result);
}

#endif // ndef SERVER
}

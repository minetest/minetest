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
#include <stdlib.h>

#ifdef GPROF
#include "prof.h"
#endif


extern int main(int argc, char *argv[]);


void android_main(android_app *app)
{
	int retval = 0;
	porting::app_global = app;

	Thread::setName("Main");

	try {
		app_dummy();
		char *argv[] = {strdup(PROJECT_NAME), NULL};
		main(ARRLEN(argv) - 1, argv);
		free(argv[0]);
	} catch (std::exception &e) {
		errorstream << "Uncaught exception in main thread: " << e.what() << std::endl;
		retval = -1;
	} catch (...) {
		errorstream << "Uncaught exception in main thread!" << std::endl;
		retval = -1;
	}

	porting::cleanupAndroid();
	infostream << "Shutting down." << std::endl;
	exit(retval);
}


static porting::AndroidEditCallback android_edit_callback;
static void *android_edit_callback_data;

/* Handler for finished message box input.
 * Intentionally NOT in namespace porting.
 */
extern "C" {
	JNIEXPORT void JNICALL Java_net_minetest_minetest_GameActivity_putEditBoxResult(
			JNIEnv *env, jclass j_this, jstring text)
	{
		const char *c_str = env->GetStringUTFChars(text, NULL);
		android_edit_callback(android_edit_callback_data, std::string(c_str));
		env->ReleaseStringUTFChars(text, c_str);
	}
}

namespace porting {

android_app *app_global;
JNIEnv *jnienv;
jclass nativeActivity;


jclass findClass(const std::string &name)
{
	FATAL_ERROR_IF(!jnienv, "findClass called before JNI environment initialization");

	static jclass cls_NativeActivity = jnienv->FindClass("android/app/NativeActivity");
	static jmethodID mt_getClassLoader = jnienv->GetMethodID(cls_NativeActivity,
			"getClassLoader", "()Ljava/lang/ClassLoader;");

	static jclass cls_ClassLoader = jnienv->FindClass("java/lang/ClassLoader");
	static jmethodID mt_findClass = jnienv->GetMethodID(cls_ClassLoader, "loadClass",
					"(Ljava/lang/String;)Ljava/lang/Class;");

	jstring strClassName = jnienv->NewStringUTF(name.c_str());

	// Call NativeActivity.getClassLoader().getClass(name)
	jobject obj = jnienv->CallObjectMethod(app_global->activity->clazz,
			mt_getClassLoader);
	return (jclass) jnienv->CallObjectMethod(obj, mt_findClass, strClassName);
}


void initAndroid()
{
	porting::jnienv = NULL;
	JavaVM *jvm = app_global->activity->vm;
	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = PROJECT_NAME_C "NativeThread";
	lJavaVMAttachArgs.group = NULL;
#ifdef NDEBUG
	// This is a ugly hack as arm v7a non debuggable builds crash without this
	// printf ... if someone finds out why please fix it!
	infostream << "Attaching native thread. " << std::endl;
#endif
	if (jvm->AttachCurrentThread(&porting::jnienv, &lJavaVMAttachArgs) == JNI_ERR) {
		errorstream << "Failed to attach native thread to JVM" << std::endl;
		exit(-1);
	}

	nativeActivity = findClass("net/minetest/minetest/GameActivity");
	FATAL_ERROR_IF(!nativeActivity, "Unable to find Java native activity class");

#ifdef GPROF
	/* in the start-up code */
	__android_log_print(ANDROID_LOG_ERROR, PROJECT_NAME_C,
			"Initializing GPROF profiler");
	monstartup("libminetest.so");
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


static std::string javaStringToUTF8(jstring js)
{
	// Get string as a UTF-8 c-string
	const char *c_str = jnienv->GetStringUTFChars(js, NULL);
	// Save it
	std::string str(c_str);
	// And free the c-string
	jnienv->ReleaseStringUTFChars(js, c_str);
	return str;
}


// Calls static method if obj is NULL
static std::string getAndroidPath(jclass cls, jobject obj, jclass cls_File,
		jmethodID mt_getAbsPath, const char *getter)
{
	// Get getter method
	jmethodID mt_getter;
	if (obj)
		mt_getter = jnienv->GetMethodID(cls, getter,
				"()Ljava/io/File;");
	else
		mt_getter = jnienv->GetStaticMethodID(cls, getter,
				"()Ljava/io/File;");

	FATAL_ERROR_IF(!mt_getter, (std::string("Failed to get Java method ") + getter).c_str());

	// Call getter
	jobject ob_file;
	if (obj)
		ob_file = jnienv->CallObjectMethod(obj, mt_getter);
	else
		ob_file = jnienv->CallStaticObjectMethod(cls, mt_getter);

	// Call getAbsolutePath
	jstring js_path = (jstring) jnienv->CallObjectMethod(ob_file,
			mt_getAbsPath);

	return javaStringToUTF8(js_path);
}


void initializePathsAndroid()
{
	// Get Environment class
	jclass cls_Env = jnienv->FindClass("android/os/Environment");
	// Get File class
	jclass cls_File = jnienv->FindClass("java/io/File");
	// Get getAbsolutePath method
	jmethodID mt_getAbsPath = jnienv->GetMethodID(cls_File,
				"getAbsolutePath", "()Ljava/lang/String;");

	path_share = getAndroidPath(nativeActivity, app_global->activity->clazz,
			cls_File, mt_getAbsPath, "getGameSharePath");
	path_user = getAndroidPath(nativeActivity, app_global->activity->clazz,
			cls_File, mt_getAbsPath, "getGameUserPath");
	path_cache = getAndroidPath(nativeActivity, app_global->activity->clazz,
			cls_File, mt_getAbsPath, "getGameCachePath");

	migrateCachePath();
}


void showInputDialog(const std::string& acceptButton, const  std::string& hint,
		const std::string& current, AndroidEditType editType,
		AndroidEditCallback cb, void *cb_data)
{
	android_edit_callback = cb;
	android_edit_callback_data = cb_data;

	static jmethodID mt_showDialog = jnienv->GetMethodID(nativeActivity,"showDialog",
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");

	FATAL_ERROR_IF(!mt_showDialog, "Unable to find Java show dialog method");

	jstring jacceptButton = jnienv->NewStringUTF(acceptButton.c_str());
	jstring jhint         = jnienv->NewStringUTF(hint.c_str());
	jstring jcurrent      = jnienv->NewStringUTF(current.c_str());
	jint    jeditType     = (int)editType;

	jnienv->CallVoidMethod(app_global->activity->clazz, mt_showDialog,
			jacceptButton, jhint, jcurrent, jeditType);
}


#ifndef SERVER
float getDisplayDensity()
{
	static bool first_run = true;
	static float value = 0;

	if (first_run) {
		jmethodID mt_getDensity = jnienv->GetMethodID(nativeActivity,
				"getDensity", "()F");

		FATAL_ERROR_IF(!mt_getDensity, "Unable to find Java getDensity method");

		value = jnienv->CallFloatMethod(app_global->activity->clazz,
				mt_getDensity);
		first_run = false;
	}

	return value;
}


v2u32 getDisplaySize()
{
	static bool first_run = true;
	static v2u32 value;

	if (first_run) {
		jmethodID mt_getDisplayWidth = jnienv->GetMethodID(nativeActivity,
				"getDisplayWidth", "()I");
		jmethodID mt_getDisplayHeight = jnienv->GetMethodID(nativeActivity,
				"getDisplayHeight", "()I");

		FATAL_ERROR_IF(!mt_getDisplayWidth, "Unable to find Java getDisplayWidth method");
		FATAL_ERROR_IF(!mt_getDisplayHeight, "Unable to find Java getDisplayHeight method");

		value.X = jnienv->CallIntMethod(app_global->activity->clazz,
				mt_getDisplayWidth);
		value.Y = jnienv->CallIntMethod(app_global->activity->clazz,
				mt_getDisplayHeight);

		first_run = false;
	}
	return value;
}
#endif // ndef SERVER
}


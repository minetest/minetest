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

#include "porting.h"
#include "porting_android.h"
#include "config.h"
#include "filesys.h"
#include "log.h"
#include <sstream>

#ifdef GPROF
#include "prof.h"
#endif

extern int main(int argc, char *argv[]);

void android_main(android_app *app)
{
	int retval = 0;
	porting::app_global = app;

	porting::setThreadName("MainThread");

	try {
		app_dummy();
		char *argv[] = { (char*) "minetest" };
		main(sizeof(argv) / sizeof(argv[0]), argv);
		}
	catch(BaseException e) {
		std::stringstream msg;
		msg << "Exception handled by main: " << e.what();
		const char* message = msg.str().c_str();
		__android_log_print(ANDROID_LOG_ERROR, PROJECT_NAME, "%s", message);
		errorstream << msg << std::endl;
		retval = -1;
	}
	catch(...) {
		__android_log_print(ANDROID_LOG_ERROR, PROJECT_NAME,
				"Some exception occured");
		errorstream << "Uncaught exception in main thread!" << std::endl;
		retval = -1;
	}

	porting::cleanupAndroid();
	errorstream << "Shutting down minetest." << std::endl;
	exit(retval);
}

/* handler for finished message box input */
/* Intentionally NOT in namespace porting */
/* TODO this doesn't work as expected, no idea why but there's a workaround   */
/* for it right now */
extern "C" {
	JNIEXPORT void JNICALL Java_org_minetest_MtNativeActivity_putMessageBoxResult(
			JNIEnv * env, jclass thiz, jstring text)
	{
		errorstream << "Java_org_minetest_MtNativeActivity_putMessageBoxResult got: "
				<< std::string((const char*)env->GetStringChars(text,0))
				<< std::endl;
	}
}

namespace porting {

std::string path_storage = DIR_DELIM "sdcard" DIR_DELIM;

android_app* app_global;
JNIEnv*      jnienv;
jclass       nativeActivity;

jclass findClass(std::string classname)
{
	if (jnienv == 0) {
		return 0;
	}

	jclass nativeactivity = jnienv->FindClass("android/app/NativeActivity");
	jmethodID getClassLoader =
			jnienv->GetMethodID(nativeactivity,"getClassLoader",
					"()Ljava/lang/ClassLoader;");
	jobject cls =
			jnienv->CallObjectMethod(app_global->activity->clazz, getClassLoader);
	jclass classLoader = jnienv->FindClass("java/lang/ClassLoader");
	jmethodID findClass =
			jnienv->GetMethodID(classLoader, "loadClass",
					"(Ljava/lang/String;)Ljava/lang/Class;");
	jstring strClassName =
			jnienv->NewStringUTF(classname.c_str());
	return (jclass) jnienv->CallObjectMethod(cls, findClass, strClassName);
}

void copyAssets()
{
	jmethodID assetcopy = jnienv->GetMethodID(nativeActivity,"copyAssets","()V");

	if (assetcopy == 0) {
		assert("porting::copyAssets unable to find copy assets method" == 0);
	}

	jnienv->CallVoidMethod(app_global->activity->clazz, assetcopy);
}

void initAndroid()
{
	porting::jnienv = NULL;
	JavaVM *jvm = app_global->activity->vm;
	JavaVMAttachArgs lJavaVMAttachArgs;
	lJavaVMAttachArgs.version = JNI_VERSION_1_6;
	lJavaVMAttachArgs.name = "MinetestNativeThread";
	lJavaVMAttachArgs.group = NULL;
#ifdef NDEBUG
	// This is a ugly hack as arm v7a non debuggable builds crash without this
	// printf ... if someone finds out why please fix it!
	infostream << "Attaching native thread. " << std::endl;
#endif
	if ( jvm->AttachCurrentThread(&porting::jnienv, &lJavaVMAttachArgs) == JNI_ERR) {
		errorstream << "Failed to attach native thread to jvm" << std::endl;
		exit(-1);
	}

	nativeActivity = findClass("org/minetest/minetest/MtNativeActivity");
	if (nativeActivity == 0) {
		errorstream <<
			"porting::initAndroid unable to find java native activity class" <<
			std::endl;
	}

#ifdef GPROF
	/* in the start-up code */
	__android_log_print(ANDROID_LOG_ERROR, PROJECT_NAME,
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

void setExternalStorageDir(JNIEnv* lJNIEnv)
{
	// Android: Retrieve ablsolute path to external storage device (sdcard)
	jclass ClassEnv      = lJNIEnv->FindClass("android/os/Environment");
	jmethodID MethodDir  =
			lJNIEnv->GetStaticMethodID(ClassEnv,
					"getExternalStorageDirectory","()Ljava/io/File;");
	jobject ObjectFile   = lJNIEnv->CallStaticObjectMethod(ClassEnv, MethodDir);
	jclass ClassFile     = lJNIEnv->FindClass("java/io/File");

	jmethodID MethodPath =
			lJNIEnv->GetMethodID(ClassFile, "getAbsolutePath",
					"()Ljava/lang/String;");
	jstring StringPath   =
			(jstring) lJNIEnv->CallObjectMethod(ObjectFile, MethodPath);

	const char *externalPath = lJNIEnv->GetStringUTFChars(StringPath, NULL);
	std::string userPath(externalPath);
	lJNIEnv->ReleaseStringUTFChars(StringPath, externalPath);

	path_storage             = userPath;
	path_user                = userPath + DIR_DELIM + PROJECT_NAME;
	path_share               = userPath + DIR_DELIM + PROJECT_NAME;
}

void showInputDialog(const std::string& acceptButton, const  std::string& hint,
		const std::string& current, int editType)
{
	jmethodID showdialog = jnienv->GetMethodID(nativeActivity,"showDialog",
		"(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)V");

	if (showdialog == 0) {
		assert("porting::showInputDialog unable to find java show dialog method" == 0);
	}

	jstring jacceptButton = jnienv->NewStringUTF(acceptButton.c_str());
	jstring jhint         = jnienv->NewStringUTF(hint.c_str());
	jstring jcurrent      = jnienv->NewStringUTF(current.c_str());
	jint    jeditType     = editType;

	jnienv->CallVoidMethod(app_global->activity->clazz, showdialog,
			jacceptButton, jhint, jcurrent, jeditType);
}

int getInputDialogState()
{
	jmethodID dialogstate = jnienv->GetMethodID(nativeActivity,
			"getDialogState", "()I");

	if (dialogstate == 0) {
		assert("porting::getInputDialogState unable to find java dialog state method" == 0);
	}

	return jnienv->CallIntMethod(app_global->activity->clazz, dialogstate);
}

std::string getInputDialogValue()
{
	jmethodID dialogvalue = jnienv->GetMethodID(nativeActivity,
			"getDialogValue", "()Ljava/lang/String;");

	if (dialogvalue == 0) {
		assert("porting::getInputDialogValue unable to find java dialog value method" == 0);
	}

	jobject result = jnienv->CallObjectMethod(app_global->activity->clazz,
			dialogvalue);

	const char* javachars = jnienv->GetStringUTFChars((jstring) result,0);
	std::string text(javachars);
	jnienv->ReleaseStringUTFChars((jstring) result, javachars);

	return text;
}

#if not defined(SERVER)
float getDisplayDensity()
{
	static bool firstrun = true;
	static float value = 0;

	if (firstrun) {
		jmethodID getDensity = jnienv->GetMethodID(nativeActivity, "getDensity",
					"()F");

		if (getDensity == 0) {
			assert("porting::getDisplayDensity unable to find java getDensity method" == 0);
		}

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

		if (getDisplayWidth == 0) {
			assert("porting::getDisplayWidth unable to find java getDisplayWidth method" == 0);
		}

		retval.X = jnienv->CallIntMethod(app_global->activity->clazz,
				getDisplayWidth);

		jmethodID getDisplayHeight = jnienv->GetMethodID(nativeActivity,
				"getDisplayHeight", "()I");

		if (getDisplayHeight == 0) {
			assert("porting::getDisplayHeight unable to find java getDisplayHeight method" == 0);
		}

		retval.Y = jnienv->CallIntMethod(app_global->activity->clazz,
				getDisplayHeight);

		firstrun = false;
	}
	return retval;
}
#endif //SERVER
}

// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#ifdef _IRR_COMPILE_WITH_ANDROID_DEVICE_

#include <jni.h>

struct android_app;

namespace irr
{
namespace jni
{

//! Minimal JNI wrapper class around android.view.KeyEvent
//! NOTE: Only functions we actually use in the engine are wrapped
//! This is currently not written to support multithreading - meaning threads are not attached/detached to the Java VM (to be discussed)
class CKeyEventWrapper
{
public:
	CKeyEventWrapper(JNIEnv* jniEnv, int action, int code);
	~CKeyEventWrapper();

	int getUnicodeChar(int metaState);

private:
	static jclass Class_KeyEvent;
	static jmethodID Method_getUnicodeChar;
	static jmethodID Method_constructor;
	JNIEnv* JniEnv;
	jobject JniKeyEvent;	// this object in java
};

} // namespace jni
} // namespace irr

#endif // _IRR_COMPILE_WITH_ANDROID_DEVICE_

// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CKeyEventWrapper.h"

#include "os.h"

namespace irr
{
namespace jni
{

jclass CKeyEventWrapper::Class_KeyEvent = 0;
jmethodID CKeyEventWrapper::Method_constructor = 0;
jmethodID CKeyEventWrapper::Method_getUnicodeChar = 0;

CKeyEventWrapper::CKeyEventWrapper(JNIEnv *jniEnv, int action, int code) :
		JniEnv(jniEnv), JniKeyEvent(0)
{
	if (JniEnv) {
		if (!Class_KeyEvent) {
			// Find java classes & functions on first call
			os::Printer::log("CKeyEventWrapper first initialize", ELL_DEBUG);
			jclass localClass = JniEnv->FindClass("android/view/KeyEvent");
			if (localClass) {
				Class_KeyEvent = reinterpret_cast<jclass>(JniEnv->NewGlobalRef(localClass));
			}

			Method_constructor = JniEnv->GetMethodID(Class_KeyEvent, "<init>", "(II)V");
			Method_getUnicodeChar = JniEnv->GetMethodID(Class_KeyEvent, "getUnicodeChar", "(I)I");
		}

		if (Class_KeyEvent && Method_constructor) {
			JniKeyEvent = JniEnv->NewObject(Class_KeyEvent, Method_constructor, action, code);
		} else {
			os::Printer::log("CKeyEventWrapper didn't find JNI classes/methods", ELL_WARNING);
		}
	}
}

CKeyEventWrapper::~CKeyEventWrapper()
{
	JniEnv->DeleteLocalRef(JniKeyEvent);
}

int CKeyEventWrapper::getUnicodeChar(int metaState)
{
	return JniEnv->CallIntMethod(JniKeyEvent, Method_getUnicodeChar, metaState);
}

} // namespace jni
} // namespace irr

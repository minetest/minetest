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
#ifndef __PORTING_ANDROID_H__
#define __PORTING_ANDROID_H__

#ifndef __ANDROID__
#error this include has to be included on android port only!
#endif

#include <jni.h>
#include <android_native_app_glue.h>
#include <android/log.h>

#include <string>

namespace porting {

enum AndroidEditType {
	AET_MULTILINE,
	AET_SINGLELINE,
	AET_PASSWORD,
};

typedef void (*AndroidEditCallback)(void*, const std::string &);

// Java app
extern android_app *app_global;

// Java <-> C++ interaction interface
extern JNIEnv *jnienv;

/// Does Android-specific initialization
void initAndroid();
void cleanupAndroid();

/** Initializes path_* variables for Android.
 * @param env Android JNI environment
 */
void initializePathsAndroid();

/** Show Android text input dialog
 * @param acceptButton text to display on accept button
 * @param hint hint to show
 * @param current initial value to display
 * @param editType type of text field
 */
void showInputDialog(const std::string& acceptButton, const std::string& hint,
		const std::string& current, AndroidEditType editType,
		AndroidEditCallback cb, void *cb_data);
}

#endif

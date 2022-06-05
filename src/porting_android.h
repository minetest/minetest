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

#pragma once

#ifndef __ANDROID__
#error this include has to be included on android port only!
#endif

#include <jni.h>
#include <android_native_app_glue.h>
#include <android/log.h>

#include <string>

namespace porting {
// java app
extern android_app *app_global;

// java <-> c++ interaction interface
extern JNIEnv *jnienv;

// do initialization required on android only
void initAndroid();

void cleanupAndroid();

/**
 * Initializes path_* variables for Android
 * @param env Android JNI environment
 */
void initializePathsAndroid();

/**
 * show text input dialog in java
 * @param acceptButton text to display on accept button
 * @param hint hint to show
 * @param current initial value to display
 * @param editType type of texfield
 * (1==multiline text input; 2==single line text input; 3=password field)
 */
void showInputDialog(const std::string &acceptButton,
					const std::string &hint, const std::string &current, int editType);

void openURIAndroid(const std::string &url);

/**
 * Opens a share intent to the file at path
 *
 * @param path
 */
void shareFileAndroid(const std::string &path);

/**
 * WORKAROUND for not working callbacks from java -> c++
 * get current state of input dialog
 */
int getInputDialogState();

/**
 * WORKAROUND for not working callbacks from java -> c++
 * get text in current input dialog
 */
std::string getInputDialogValue();

#ifndef SERVER
float getDisplayDensity();
v2u32 getDisplaySize();
#endif
}

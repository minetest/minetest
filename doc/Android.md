**Document based on Minetest `5.6.1` Android build**
# Minetest Android build
Minetest for Android are the same Minetest on Windows but compiled for Android, not seperate version
## Controls
The Android port doesn't support everything you can do on PC due to the
limited capabilities of common devices. What can be done is described
below:

While you're playing the game normally (that is, no menu or inventory is
shown), the following controls are available:
* Look around: touch screen and slide finger
* Double tap: Place a node
* Long tap: Dig node or use the holding item
* Press back: Pause menu
* Touch buttons: Press button
* Buttons:

1. Left upper corner: Chat
2. Right lower corner: Jump
3. Right lower corner: Crouch
4. Left lower corner (Joystick): Walk/step...
5. Left lower corner: Display inventory

When a menu or inventory is displayed:
* Double tap outside menu area: Close menu
* Press back: Close menu
* Tap on an item stack: Select that stack
* Tap on an empty slot: If a stack are selected already, that stack is placed here
* Drag and drop: Touch stack and hold finger down, move the stack to another
  slot, tap another finger while keeping first finger on screen
  --> places a single item from dragged stack into current (first touched) slot. If a stack is selected, the stack will be split as half and one of the splitted stack will be selected
### Limitaions
* Android player have to double tap to place node, this can be annoying in some game/mod
* Some old Android device only support 2 touch at a time, some game/mod contain button combination that need 3 touch (example: jump + Aux1 + hold)
* Complicated control like pick up an cart in MTG can be difficult or impossible on Android device
## File Path
There are some settings especially useful for Android users. Minetest's config
file can usually be found at:

* Before 5.4.2: ``/sdcard/Minetest/`` or ``/storage/emulated/0/`` if stored in device, if stored in SD card: ``/storage/emulated/(varying folder name)/``
* After 5.4.2: ``/sdcard/Android/data/net.minetest.minetest/`` or ``/storage/emulated/0/Android/data/net.minetest.minetest/`` if stored in device, if stored in SD card: ``/storage/emulated/(varying folder name)/Android/data/net.minetest.minetest/``
* [Learn more about Android directory](https://wiki.minetest.net/Accessing_Android_Data_Directory)
## Useful settings
### gui_scaling
this is a user-specified scaling factor for the GUI in case main menu is too big or small on your device, try changing this value.
### mapblock_limit
Mobile device generally have less RAM than PC, this setting limit how many mapblock can keep in RAM
### fps_limit
this setting limit max FPS (Frame per second). Default value is 60, which lowest Android device screen refresh rate commonly found, but if you're using an device have lower refresh rate, change this
## Requirements
Read this before try installing Minetest!
### CPU
Minetest currently support the following CPU architect:
1. ARM v7
2. ARM v8
3. x86
4. x86_64

CPU architect similar to ARM or x86 can ***probally*** able to play Minetest but are not recommended!
### Minimum:
1. Graphic: OpenGL ES 1.0
2. Android version: Android 4.1 (API Level 14)
3. Empty RAM: 500 MB
4. Empty Storage: 20 MB
* **Able to play Minetest Game and some games with light weight mod**
* **No advanced shader on OpengGL ES 1!**
### Recommended:
1. Graphic: OpenGL ES 2.0
2. Android version: No lowest recommended version
3. Empty RAM: 850 MB
4. Empty Storage: 480 MB
* **Able to play to play most of game with most of mod**
* **OpenGL ES 2 can use advanced shader!**
## Rendering
Unlike on PC, Android device use OpenGL ES which less powerful than OpenGL. Some shader setting can't be used on OpenGL ES. And because of Android device use OpenGL ES, change the graphic driver setting to OpenGL result in **CRASH**
## Building Requirements
In order to build, your PC has to be set up to build Minetest in the usual
manner (see the regular Minetest documentation for how to get this done).
In addition to what is required for Minetest in general, you will need the
following software packages. The version number in parenthesis denotes the
version that was tested at the time this README was drafted; newer/older
versions may or may not work.

* Android SDK 29
* Android NDK r21
* Android Studio 3 [optional]

Additionally, you'll need to have an Internet connection available on the
build system, as the Android build will download some source packages.

## Build
The new build system Minetest Android is fully functional and is designed to
speed up and simplify the work, as well as adding the possibility of
cross-platform build.
You can use `./gradlew assemblerelease` or `./gradlew assembledebug` from the
command line or use Android Studio and click the build button.

When using gradlew, the newest NDK will be downloaded and installed
automatically. Or you can create a `local.properties` file and specify
`sdk.dir` and `ndk.dir` yourself.

* In order to make a release build you'll have to have a keystore setup to sign
  the resulting apk package. How this is done is not part of this README. There
  are different tutorials on the web explaining how to do it
  - choose one yourself.

* Once your keystore is setup, enter the android subdirectory and create a new
  file "ant.properties" there. Add following lines to that file:

  > key.store=<path to your keystore>
  > key.alias=Minetest

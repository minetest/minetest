# Minetest Android Build

All Minetest builds, including the Android variant, are based on the same code.
However, additional Java code is used for proper Android integration.

## Controls

Compared to Minetest binaries for PC, the Android port has limited functionality
due to limited capabilities of common devices. What can be done is described below:

While you're playing the game normally (that is, no menu or inventory is
shown), the following controls are available:

* Look around: Touch screen and slide finger
* Tap: Place a node, punch an object, or use the wielded item (default)
* Long tap: Dig a node or use the wielded item
* Press back: Pause menu
* Touch buttons: Press button
* Buttons:
    1. Upper-left corner: Chat
    2. Lower-right corner: Jump
    3. Lower-right corner: Crouch
    4. Lower-left corner (Joystick): Walk
    5. Lower-left corner: Display inventory

When a menu or inventory is displayed:
* Double tap outside menu area / Press back: Close menu
* Tap on an item stack: Select that stack
* Tap on an empty slot: If a stack is selected already, that stack is placed here
* Drag and drop: Touch stack and hold finger down, move the stack to another
  slot, tap another finger while keeping first finger on screen
  --> places a single item from dragged stack into current (first touched) slot. If a stack is selected, the stack will be split as half and one of the splitted stack will be selected

### Limitations

* Complicated controls can be difficult or impossible on Android devices
* Some old Android devices only support 2 touches at a time, some games/mods contain button combinations that need 3 touches (example: jump + Aux1 + hold)

## File Path

There are some settings especially useful for Android users. The Minetest-wide
configuration file can usually be found at:

* Before 5.4.2:
    * `/sdcard/Minetest/` or `/storage/emulated/0/` if stored on the device
    * `/storage/emulated/(varying folder name)/` if stored on an SD card
* After 5.4.2:
    * `/sdcard/Android/data/net.minetest.minetest/` or `/storage/emulated/0/Android/data/net.minetest.minetest/` if stored on the device
    * `/storage/emulated/(varying folder name)/Android/data/net.minetest.minetest/` if stored on the SD card
* [Learn more about Android directory](https://wiki.minetest.net/Accessing_Android_Data_Directory)

## Useful Settings

### `gui_scaling`

This is a user-specified scaling factor for the GUI. If the main menu is too big or small on your device, try changing
this value.

### `mapblock_limit`

Mobile devices generally have less RAM than PC, this setting limits how many mapblock can be kept in RAM.

### `fps_limit`

This setting limits maximum FPS. The default value is `60`, which is the lowest commonly found
Android refresh rate, but if you're using a device with a lower refresh rate, change this.

## Rendering

Unlike on PC, Android devices use OpenGL ES which less powerful than OpenGL, thus
some shader settings cannot be used on OpenGL ES.
Changing the graphic driver setting to OpenGL will result in undesirable behavior.

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

* In order to make a release build, you'll have to have a keystore setup to sign
  the resulting APK package. How this is done is not part of this README. There
  are tutorials online that explain it.
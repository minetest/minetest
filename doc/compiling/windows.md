# Compiling on Windows using MSVC

If you're just creating mods or games with Luanti, you do not need to compile Luanti. Instead, follow Ruben Wardy's [Luanti Modding Book](https://rubenwardy.gitlab.io/minetest_modding_book) to get started modding. Compiling Luanti is only required if you plan to modify the Luanti engine itself.

## Requirements

-   [Visual Studio 2015 or newer](https://visualstudio.microsoft.com) including "Desktop development with C++"
-   [CMake](https://cmake.org/download/)
-   [vcpkg](https://github.com/Microsoft/vcpkg) (included with Visual Studio)
-   [Git](https://git-scm.com/downloads)

### Visual Studio with C++

1. Install Visual Studio (VS) from [visualstudio.microsoft.com](https://visualstudio.microsoft.com)
1. In the VS installer, select "Desktop development with C++":

    ![VS installer showing desktop development with C++ selected](assets/vs-installer.png)

1. Confirm the installation.

This will install the C++ compiler used in later steps. VS is also the recommended IDE for Luanti.

You may notice that the C++ tools include CMake, but it should also be installed separately for compatibility with Luanti.

### CMake

Install from [cmake.org/download](https://cmake.org/download/). Once installed, you should be able to run `cmake-gui` from the start menu:

![cmake-gui in Windows start menu shows app result](./assets/cmake-gui-search.png)

## Install Luanti dependencies

### Optional dependencies

Some optional dependencies are recommended for advanced development. You can add them to `vcpkg.json` to install them along with the required dependencies.

-   `curl` is highly recommended, as it's required to read the serverlist; `curl[winssl]` is required to use the content store.
-   `openal-soft`, `libvorbis` and `libogg` are optional, but required to use sound.
-   `luajit` is optional, it replaces the integrated Lua interpreter with a faster just-in-time interpreter.
-   `gmp` and `jsoncpp` are optional, otherwise the bundled versions will be compiled
-   `gettext` is optional, but required to use translations.

### Install dependencies

1. Start up the CMake GUI (Win > search "cmake-gui" > open)
1. Select **Browse Source...** and select `path/to/minetest` (where you've cloned the repo)
1. Select **Browse Build...** and select `path/to/minetest/build` (a new folder that CMake will prompt to create)
1. Select **Configure**
1. Choose the right Visual Studio version and target platform. Currently, Luanti uses Visual Studio 16 2019, but newer VS versions should work as well. The VS version has to match the version of the installed dependencies.
1. Choose **Specify toolchain file for cross-compiling**
1. Click **Next**
1. Select the vcpkg toolchain file, e.g. `C:/Program Files/Microsoft Visual Studio/2022/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake`.
1. Click Finish
1. Wait until CMake generates the cache file (this may take about 10-30 minutes, depending on your device)
1. If there are any errors, solve them and hit **Configure**
1. Click **Generate**
1. Click **Open Project**

## Compile Luanti

There are two ways to compile Luanti: via Visual Studio or via CLI.

### Compile in Visual Studio

14. Compile Luanti inside Visual Studio.
    -   If you get "Unable to start program '...\x64\Debug\ALL_BUILD'. Access is denied", try compiling via the CLI instead.

### Compile via CLI

While in the `path/to/minetest` folder, run the following command:

```
cmake --build build --config Release
```

## Running Luanti

You should be able to run `path/to/minetest/bin/Release/luanti.exe` and start up the Development Test game :)

## Windows Installer using WiX Toolset

Requirements:

-   [Visual Studio 2017](https://visualstudio.microsoft.com/)
-   [WiX Toolset](https://wixtoolset.org/)

In the Visual Studio 2017 Installer select **Optional Features -> WiX Toolset**.

Build the binaries as described above, but make sure you unselect `RUN_IN_PLACE`.

Open the generated project file with Visual Studio. Right-click **Package** and choose **Generate**.
It may take some minutes to generate the installer.

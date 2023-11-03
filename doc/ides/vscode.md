# [Visual Studio Code](https://code.visualstudio.com)

VSCode suppport for C/C++ and CMake is provided by
the [Microsoft C/C++ extension pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack).
You can install it from the VSCode extensions tab.

If you use a unofficial VSCode distribution like [VSCodium](https://vscodium.com), you will need to install the
extension pack manually by downloading the VSIX files and going to `Extensions > ... > Install from VSIX`.

CMake support for VSCode uses CMake presets provided by the project by default.

When you open the Minetest folder with VSCode, you should get a quick pick asking you for the default preset.

![VSCode CMake Preset Selection](vscode_cmake_preset_selection.png)

You can use the bottom bar to change the CMake profile, change the build target, build, run and debug (running/debugging doesn't build first).

![VSCode Toolbar](vscode_toolbar.png)

Like most of the VSCode experience, it may be faster to use commands directly (most of the VSCode UI just trigger commands).

| Command Name                     | Usecase                          |
|----------------------------------|----------------------------------|
| `CMake: Select Configure Preset` | Change the current CMake profile |
| `CMake: Set Build Target`        | Change the current build target  |
| `CMake: Build`                   | Build current target             |
| `CMake: Clean`                   | Clean build files                |
| `CMake: Run Without Debugging`   | Run selected run target          |
| `CMake: Debug`                   | Debug selected run target        |

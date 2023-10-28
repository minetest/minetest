# [Jetbrains IntellIJ CLion](https://www.jetbrains.com/clion)

## Linux

When opening the folder for the first time, select "Open as CMake project"

The IDE will open the folder and display the open project wizard:

![Open Project Wizard](jetbrains_open_project_wizard.png)

In this wizard, you can create profiles that will be used to build the project.

By default only a profile with the `Debug` build type is created. You can create a new profile by clicking on the "+" button if you want to create builds with the `Release` build type for exemple.

Make sure to switch the Generator to `Let CMake decide` or `Unix Makefiles`.

You can change the build options passed to Make, for exemple if you want to change the number of threads used to build the project (`-j`).

The CMake profiles can be later configured in `"Settings" > "Build, Execution, Deployment" > "CMake"`.

The configuration of the CMake project created the CMake run configurations that can be accessed from the toolbar to build, run and debug the program. At the left there is the dropdown to change the CMake profile.

![Jetbrains Run Toolbar](jetbrains_run_toolbar.png)

From the recent project view when you start CLion, you can rightclick the project and change it's icon to make it easier to navigate your projects.

## Windows

Under Windows, you need to install the [Visual Studio](https://visualstudio.microsoft.com) compiler.

From the Visual Studio installer, you need to install the `Desktop development with C++` Workload. CMake is bundled in CLion, but if you want to use CMake from anywhere you need to make sure the `C++ CMake tools for Windows` component is included in the installation details panel.

The open project wisard will look like this at first, you can pick `Visual Studio` from the list and press next:

![image](https://github.com/AFCMS/minetest/assets/61794590/2f4ed350-6451-45ac-b553-30310cce798f)

Then, the process is roughly similar to Linux, you just need to pick `Visual Studio` as toolchain.

![image](https://github.com/AFCMS/minetest/assets/61794590/7cc87b5e-c7cf-444d-aa2e-760b43241a40)

After that, you need to let CLion use `vcpkg` for dependencies.

Go to `View > Tool Windows > Vcpkg` and click the add button. I will open a popup allowing you to add a Vcpkg installation. By default it will download a new one, but if you already have one installed you can select your installation folder.



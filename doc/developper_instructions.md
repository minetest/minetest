# Developper Instructions

## IntellIJ CLion

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
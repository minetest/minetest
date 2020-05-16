Source code of the Irrlicht Engine

The complete source of the Irrlicht Engine can be found when decompressing
the .zip file included in this directory. 
Please note that YOU DO NOT NEED THIS SOURCE to develop 3d applications with
the Irrlicht Engine. Instead, please use the .dll in the \bin directory, the 
.lib in the \lib directory and the header files in the \include directory.

You will find a good tutorial how to set up your development environment and to
use the engine in the \examples directory. (Try 1.helloworld)

The source of the engine is only included because of the following reasons:

	- To let developers be able to debug the engine.
	- To let developers be able to make changes to the engine.
	- To let developers be able to compile their own versions of the engine.
	
	
	
HOW TO COMPILE THE ENGINE WITH LINUX

If you wish to compile the engine in linux yourself, unzip the source source.zip
file in the \source directory. Run a 'make' in the now existing new subfolder 'Irrlicht'.
After this, you should be able to make all example applications in \examples.
Then just start an X Server and run them, from the directory where they are.

If you get a compiling/linking problem like 

 undefined reference to `glXGetProcAddress'
 
Then there are several solutions: 
A) This disables the use of OpenGL extensions:
  Open the file IrrCompileConfig.h, comment out _IRR_OPENGL_USE_EXTPOINTER_,
  and recompile Irrlicht using 
  make clean
  make
B) Replace all occurrences of 'glXGetProcAddress' with 'glXGetProcAddressARB' and run a
   make
   This will solve the issue but keep the OpenGL extension enabled.
   

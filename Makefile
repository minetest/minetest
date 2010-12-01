# Makefile for Irrlicht Examples
# It's usually sufficient to change just the target name and source file list
# and be sure that CXX is set to a valid compiler
TARGET = test
SOURCE_FILES = voxel.cpp mapblockobject.cpp inventory.cpp debug.cpp serialization.cpp light.cpp filesys.cpp connection.cpp environment.cpp client.cpp server.cpp socket.cpp mapblock.cpp mapsector.cpp heightmap.cpp map.cpp player.cpp utility.cpp main.cpp test.cpp
SOURCES = $(addprefix src/, $(SOURCE_FILES))
OBJECTS = $(SOURCES:.cpp=.o)
FASTTARGET = fasttest

IRRLICHTPATH = ../irrlicht/irrlicht-1.7.1
JTHREADPATH = ../jthread/jthread-1.2.1

CPPFLAGS = -I$(IRRLICHTPATH)/include -I/usr/X11R6/include -I$(JTHREADPATH)/src

#CXXFLAGS = -O2 -ffast-math -Wall -fomit-frame-pointer -pipe
#CXXFLAGS = -O2 -ffast-math -Wall -g -pipe
#CXXFLAGS = -O1 -ffast-math -Wall -g
CXXFLAGS = -Wall -g -O0

#CXXFLAGS = -O3 -ffast-math -Wall
#CXXFLAGS = -O3 -ffast-math -Wall -g
#CXXFLAGS = -O2 -ffast-math -Wall -g

FASTCXXFLAGS = -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops -mtune=i686
#FASTCXXFLAGS = -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops -mtune=i686 -fwhole-program

#Default target

all: all_linux

ifeq ($(HOSTTYPE), x86_64)
LIBSELECT=64
endif

# Target specific settings

all_linux fast_linux: LDFLAGS = -L/usr/X11R6/lib$(LIBSELECT) -L$(IRRLICHTPATH)/lib/Linux -L$(JTHREADPATH)/src/.libs -lIrrlicht -lGL -lXxf86vm -lXext -lX11 -ljthread
all_linux fast_linux clean_linux: SYSTEM=Linux

all_win32: LDFLAGS = -L$(IRRLICHTPATH)/lib/Win32-gcc -L$(JTHREADPATH)/Debug -lIrrlicht -lopengl32 -lm -ljthread
all_win32 clean_win32: SYSTEM=Win32-gcc
all_win32 clean_win32: SUF=.exe

# Name of the binary - only valid for targets which set SYSTEM

DESTPATH = bin/$(TARGET)$(SUF)
FASTDESTPATH = bin/$(FASTTARGET)$(SUF)

# Build commands

all_linux all_win32: $(DESTPATH)

fast_linux: $(FASTDESTPATH)

$(FASTDESTPATH): $(SOURCES)
	$(CXX) -o $(FASTDESTPATH) $(SOURCES) $(CPPFLAGS) $(FASTCXXFLAGS) $(LDFLAGS) -DUNITTEST_DISABLE
	@# Errno doesn't work ("error: ‘__errno_location’ was not declared in this scope")
	@#cat $(SOURCES) | $(CXX) -o $(FASTDESTPATH) -x c++ - -Isrc/ $(CPPFLAGS) $(FASTCXXFLAGS) $(LDFLAGS) -DUNITTEST_DISABLE -DDISABLE_ERRNO

$(DESTPATH): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

.cpp.o:
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

clean: clean_linux clean_win32 clean_fast_linux

clean_linux clean_win32:
	@$(RM) $(OBJECTS) $(DESTPATH)

clean_fast_linux:
	@$(RM) $(FASTDESTPATH)

.PHONY: all all_win32 clean clean_linux clean_win32

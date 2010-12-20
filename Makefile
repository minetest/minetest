# Makefile for Irrlicht Examples
# It's usually sufficient to change just the target name and source file list
# and be sure that CXX is set to a valid compiler
TARGET = test
SOURCE_FILES = irrlichtwrapper.cpp guiPauseMenu.cpp defaultsettings.cpp mapnode.cpp tile.cpp voxel.cpp mapblockobject.cpp inventory.cpp debug.cpp serialization.cpp light.cpp filesys.cpp connection.cpp environment.cpp client.cpp server.cpp socket.cpp mapblock.cpp mapsector.cpp heightmap.cpp map.cpp player.cpp utility.cpp main.cpp test.cpp
SOURCES = $(addprefix src/, $(SOURCE_FILES))
BUILD_DIR = build
OBJECTS = $(addprefix $(BUILD_DIR)/, $(SOURCE_FILES:.cpp=.o))
#OBJECTS = $(SOURCES:.cpp=.o)

FAST_TARGET = fasttest

SERVER_TARGET = server
SERVER_SOURCE_FILES = defaultsettings.cpp mapnode.cpp voxel.cpp mapblockobject.cpp inventory.cpp debug.cpp serialization.cpp light.cpp filesys.cpp connection.cpp environment.cpp server.cpp socket.cpp mapblock.cpp mapsector.cpp heightmap.cpp map.cpp player.cpp utility.cpp servermain.cpp test.cpp
SERVER_SOURCES = $(addprefix src/, $(SERVER_SOURCE_FILES))
SERVER_BUILD_DIR = serverbuild
SERVER_OBJECTS = $(addprefix $(SERVER_BUILD_DIR)/, $(SERVER_SOURCE_FILES:.cpp=.o))
#SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)

IRRLICHTPATH = ../irrlicht/irrlicht-1.7.1
JTHREADPATH = ../jthread/jthread-1.2.1

#CXXFLAGS = -O2 -ffast-math -Wall -fomit-frame-pointer -pipe
CXXFLAGS = -O2 -ffast-math -Wall -g -pipe
#CXXFLAGS = -O1 -ffast-math -Wall -g
#CXXFLAGS = -Wall -g -O0

#FAST_CXXFLAGS = -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops -mtune=i686

#Default target

all: all_linux

ifeq ($(HOSTTYPE), x86_64)
LIBSELECT=64
endif

# Target specific settings

all_linux fast_linux: LDFLAGS = -L/usr/X11R6/lib$(LIBSELECT) -L$(IRRLICHTPATH)/lib/Linux -L$(JTHREADPATH)/src/.libs -lIrrlicht -lGL -lXxf86vm -lXext -lX11 -ljthread -lz
all_linux fast_linux: CPPFLAGS = -I$(IRRLICHTPATH)/include -I/usr/X11R6/include -I$(JTHREADPATH)/src
fast_linux server_linux: CXXFLAGS = -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops -mtune=i686
server_linux: LDFLAGS = -L$(JTHREADPATH)/src/.libs -ljthread -lz -lpthread
server_linux: CPPFLAGS = -I$(IRRLICHTPATH)/include -I/usr/X11R6/include -I$(JTHREADPATH)/src -DSERVER
all_linux fast_linux clean_linux: SYSTEM=Linux

# These are out of date
all_win32: LDFLAGS = -L$(IRRLICHTPATH)/lib/Win32-gcc -L$(JTHREADPATH)/Debug -lIrrlicht -lopengl32 -lm -ljthread
all_win32 clean_win32: SYSTEM=Win32-gcc
all_win32 clean_win32: SUF=.exe

# Name of the binary - only valid for targets which set SYSTEM

DESTPATH = bin/$(TARGET)$(SUF)
FAST_DESTPATH = bin/$(FAST_TARGET)$(SUF)
SERVER_DESTPATH = bin/$(SERVER_TARGET)$(SUF)

# Build commands

all_linux all_win32: $(BUILD_DIR) $(DESTPATH)
fast_linux: $(BUILD_DIR) $(FAST_DESTPATH)
server_linux: $(SERVER_BUILD_DIR) $(SERVER_DESTPATH)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
$(SERVER_BUILD_DIR):
	mkdir -p $(SERVER_BUILD_DIR)

$(DESTPATH): $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

$(FAST_DESTPATH): $(SOURCES)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) -DUNITTEST_DISABLE

$(SERVER_DESTPATH): $(SERVER_OBJECTS)
	$(CXX) -o $@ $(SERVER_OBJECTS) $(LDFLAGS) -DSERVER -DUNITTEST_DISABLE

$(BUILD_DIR)/%.o: src/%.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

$(SERVER_BUILD_DIR)/%.o: src/%.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

clean: clean_linux clean_win32 clean_fast_linux clean_server_linux

clean_linux clean_win32:
	@$(RM) $(OBJECTS) $(DESTPATH)

clean_fast_linux:
	@$(RM) $(OBJECTS) $(FAST_DESTPATH)

clean_server_linux:
	@$(RM) $(SERVER_OBJECTS) $(SERVER_DESTPATH)

.PHONY: all all_win32 clean clean_linux clean_win32 clean_fast_linux clean_server_linux

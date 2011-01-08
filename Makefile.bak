# Makefile for Irrlicht Examples
# It's usually sufficient to change just the target name and source file list
# and be sure that CXX is set to a valid compiler
SOURCE_FILES = porting.cpp guiMessageMenu.cpp materials.cpp guiTextInputMenu.cpp guiInventoryMenu.cpp irrlichtwrapper.cpp guiPauseMenu.cpp defaultsettings.cpp mapnode.cpp tile.cpp voxel.cpp mapblockobject.cpp inventory.cpp debug.cpp serialization.cpp light.cpp filesys.cpp connection.cpp environment.cpp client.cpp server.cpp socket.cpp mapblock.cpp mapsector.cpp heightmap.cpp map.cpp player.cpp utility.cpp main.cpp test.cpp

DEBUG_TARGET = debugtest
DEBUG_SOURCES = $(addprefix src/, $(SOURCE_FILES))
DEBUG_BUILD_DIR = debugbuild
DEBUG_OBJECTS = $(addprefix $(DEBUG_BUILD_DIR)/, $(SOURCE_FILES:.cpp=.o))

FAST_TARGET = fasttest
FAST_SOURCES = $(addprefix src/, $(SOURCE_FILES))
FAST_BUILD_DIR = fastbuild
FAST_OBJECTS = $(addprefix $(FAST_BUILD_DIR)/, $(SOURCE_FILES:.cpp=.o))

SERVER_TARGET = server
SERVER_SOURCE_FILES = porting.cpp materials.cpp defaultsettings.cpp mapnode.cpp voxel.cpp mapblockobject.cpp inventory.cpp debug.cpp serialization.cpp light.cpp filesys.cpp connection.cpp environment.cpp server.cpp socket.cpp mapblock.cpp mapsector.cpp heightmap.cpp map.cpp player.cpp utility.cpp servermain.cpp test.cpp
SERVER_SOURCES = $(addprefix src/, $(SERVER_SOURCE_FILES))
SERVER_BUILD_DIR = serverbuild
SERVER_OBJECTS = $(addprefix $(SERVER_BUILD_DIR)/, $(SERVER_SOURCE_FILES:.cpp=.o))

IRRLICHTPATH = ../irrlicht/irrlicht-1.7.1
JTHREADPATH = ../jthread/jthread-1.2.1


all: fast

ifeq ($(HOSTTYPE), x86_64)
LIBSELECT=64
endif

debug: CXXFLAGS = -Wall -g -O1
debug: CPPFLAGS = -I$(IRRLICHTPATH)/include -I/usr/X11R6/include -I$(JTHREADPATH)/src -DDEBUG -DRUN_IN_PLACE
debug: LDFLAGS = -L/usr/X11R6/lib$(LIBSELECT) -L$(IRRLICHTPATH)/lib/Linux -L$(JTHREADPATH)/src/.libs -lIrrlicht -lGL -lXxf86vm -lXext -lX11 -ljthread -lz

fast: CXXFLAGS = -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops -mtune=i686
fast: CPPFLAGS = -I$(IRRLICHTPATH)/include -I/usr/X11R6/include -I$(JTHREADPATH)/src -DUNITTEST_DISABLE -DRUN_IN_PLACE
fast: LDFLAGS = -L/usr/X11R6/lib$(LIBSELECT) -L$(IRRLICHTPATH)/lib/Linux -L$(JTHREADPATH)/src/.libs -lIrrlicht -lGL -lXxf86vm -lXext -lX11 -ljthread -lz

server: CXXFLAGS = -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops -mtune=i686
server: CPPFLAGS = -I$(IRRLICHTPATH)/include -I/usr/X11R6/include -I$(JTHREADPATH)/src -DSERVER -DUNITTEST_DISABLE -DRUN_IN_PLACE
server: LDFLAGS = -L$(JTHREADPATH)/src/.libs -ljthread -lz -lpthread

DEBUG_DESTPATH = bin/$(DEBUG_TARGET)
FAST_DESTPATH = bin/$(FAST_TARGET)
SERVER_DESTPATH = bin/$(SERVER_TARGET)

# Build commands

debug: $(DEBUG_BUILD_DIR) $(DEBUG_DESTPATH)
fast: $(FAST_BUILD_DIR) $(FAST_DESTPATH)
server: $(SERVER_BUILD_DIR) $(SERVER_DESTPATH)

$(DEBUG_BUILD_DIR):
	mkdir -p $(DEBUG_BUILD_DIR)
$(FAST_BUILD_DIR):
	mkdir -p $(FAST_BUILD_DIR)
$(SERVER_BUILD_DIR):
	mkdir -p $(SERVER_BUILD_DIR)

$(DEBUG_DESTPATH): $(DEBUG_OBJECTS)
	$(CXX) -o $@ $(DEBUG_OBJECTS) $(LDFLAGS)

$(FAST_DESTPATH): $(FAST_OBJECTS)
	$(CXX) -o $@ $(FAST_OBJECTS) $(LDFLAGS)

$(SERVER_DESTPATH): $(SERVER_OBJECTS)
	$(CXX) -o $@ $(SERVER_OBJECTS) $(LDFLAGS)

$(DEBUG_BUILD_DIR)/%.o: src/%.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

$(FAST_BUILD_DIR)/%.o: src/%.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

$(SERVER_BUILD_DIR)/%.o: src/%.cpp
	$(CXX) -c -o $@ $< $(CPPFLAGS) $(CXXFLAGS)

clean: clean_debug clean_fast clean_server

clean_debug:
	@$(RM) $(DEBUG_OBJECTS) $(DEBUG_DESTPATH)

clean_fast:
	@$(RM) $(FAST_OBJECTS) $(FAST_DESTPATH)

clean_server:
	@$(RM) $(SERVER_OBJECTS) $(SERVER_DESTPATH)

.PHONY: all all_win32 clean clean_debug clean_win32 clean_fast clean_server

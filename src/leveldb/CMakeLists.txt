set(CMAKE_LEGACY_CYGWIN_WIN32 0)
cmake_minimum_required(VERSION 2.8)

PROJECT(leveldb)
# version
set(LIB_MAJOR 0)
set(LIB_MINOR 0)
set(LIB_RELEASE 1)
set(CMAKE_INSTALL_SO_NO_EXE "0")

include(CheckIncludeFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR} include)


# STATIC/DYNAMIC LIBRARY INCLUSION
if(MINGW OR WIN32)
  OPTION(STATIC "build a static binaries. This is mainly intended for the win32-package and the MACOSX-bundle." ON )
else(MINGW OR WIN32)
  OPTION(STATIC "build a static binaries. This is mainly intended for the win32-package and the MACOSX-bundle." OFF )
endif()



# OS detection
if(WIN32)
    add_definitions(-DLEVELDB_PLATFORM_WINDOWS -DOS_WIN -DWIN32_LEAN_AND_MEAN)
    if(MINGW)
add_definitions(-DMINGW)
    endif()
    if(MSVC)
        add_definitions(-DCOMPILER_MSVC)
    endif()
    set(PORT_SRCS port/port_win.cc util/env_boost.cc)
    set(CIF_SRCS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    add_definitions(-DLEVELDB_PLATFORM_POSIX -DOS_FREEBSD)
    set(PORT_SRCS port/port_posix.cc util/env_posix.cc)
    set(CIF_SRCS db/c.cc)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_definitions(-DLEVELDB_PLATFORM_POSIX -DOS_LINUX)
    # link pthread
    set(PORT_SRCS port/port_posix.cc util/env_posix.cc)
    set(CIF_SRCS db/c.cc)
elseif(CYGWIN)
    add_definitions(-DLEVELDB_PLATFORM_POSIX -DOS_LINUX)
    set(PORT_SRCS port/port_posix.cc util/env_posix.cc)
    set(CIF_SRCS db/c.cc)
elseif(APPLE)
    add_definitions(-DLEVELDB_PLATFORM_POSIX -DOS_MACOSX)
    set(PORT_SRCS port/port_posix.cc util/env_posix.cc)
    set(CIF_SRCS db/c.cc)
endif()

# FIXME: check cstdatomic here..

set(SRCS
    db/builder.cc
    db/db_impl.cc
    db/db_iter.cc
    db/filename.cc
    db/dbformat.cc
    db/log_reader.cc
    db/log_writer.cc
    db/memtable.cc
    db/repair.cc
    db/table_cache.cc
    db/version_edit.cc
    db/version_set.cc
    db/write_batch.cc
    table/block.cc
    table/block_builder.cc
    table/filter_block.cc
    table/format.cc
    table/iterator.cc
    table/merger.cc
    table/table.cc
    table/table_builder.cc
    table/two_level_iterator.cc
    util/env.cc
    util/arena.cc
    util/bloom.cc
    util/cache.cc
    util/coding.cc
    util/comparator.cc
    util/crc32c.cc
    util/filter_policy.cc
    util/hash.cc
    util/histogram.cc
    util/logging.cc
    util/options.cc
    util/status.cc
    ${PORT_SRCS}
    ${CIF_SRCS}
    )

set(HDRS
    include/leveldb/c.h
    include/leveldb/cache.h
    include/leveldb/comparator.h
    include/leveldb/db.h
    include/leveldb/env.h
    include/leveldb/iterator.h
    include/leveldb/options.h
    include/leveldb/slice.h
    include/leveldb/status.h
    include/leveldb/table.h
    include/leveldb/table_builder.h
    include/leveldb/write_batch.h)
    
set(TESTHARNESS_SRCS
    util/testutil.cc
    util/testharness.cc)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(BUILDSTATIC)
  add_library(leveldb STATIC ${SRCS})
else(BUILDSTATIC)
  add_library(leveldb SHARED ${SRCS})
endif()

set(_soversion ${LIB_MAJOR}.${LIB_MINOR}.${LIB_RELEASE})
set_target_properties(leveldb PROPERTIES SOVERSION ${LIB_MAJOR} VERSION ${_soversion})
target_link_libraries(leveldb pthread)
SET (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

IF(MINGW)
  set_property(TARGET leveldb PROPERTY LINK_SEARCH_END_STATIC ON)
  set_target_properties(leveldb PROPERTIES LINK_FLAGS "-static-libgcc")
ENDIF()

INSTALL(FILES ${HDRS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/leveldb)
if(MINGW OR WIN32)
  INSTALL(TARGETS leveldb DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
else(MINGW OR wIN32)
  INSTALL(TARGETS leveldb LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
endif()

if(TESTDB)
  ENABLE_TESTING()
  # TESTS
  add_library(leveldb-testharness STATIC ${TESTHARNESS_SRCS})

  macro(leveldb_test ${nam})
  endmacro()

  macro(leveldb_tests)
    foreach(t ${ARGN})
        get_filename_component(_tname ${t} NAME_WE)
        add_executable(${_tname} ${t})
        target_link_libraries(${_tname}
            leveldb-testharness
            leveldb
pthread
            )
    endforeach()
  endmacro()

  leveldb_tests(
    db/db_bench.cc
    util/arena_test.cc
    db/c_test.c
    util/cache_test.cc
    util/coding_test.cc
    db/corruption_test.cc
    util/crc32c_test.cc
    db/db_test.cc
    db/dbformat_test.cc
    util/env_test.cc
    db/log_test.cc
    table/table_test.cc
    db/skiplist_test.cc
    db/version_edit_test.cc
    db/version_set_test.cc
    db/write_batch_test.cc)
endif()
# TODO: memenv_test

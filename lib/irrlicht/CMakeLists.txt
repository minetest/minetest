include_directories(${IRRLICHT_INCLUDE_DIR})
add_subdirectory(source)
add_library(irrlicht ${IRRLICHT_SRCS})

target_link_libraries(irrlicht)

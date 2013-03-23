include_directories(
  ${IRRLICHT_INCLUDE_DIR}
  ${FREETYPE_INCLUDE_DIRS}
)

# CGUITTFont authors, y u no include headers you use?
#   Do not add CGUITTFont.cpp to the line below.
#   xCGUITTFont.cpp is a wrapper file that includes
#   additional required headers.
add_library(cguittfont xCGUITTFont.cpp)

target_link_libraries(
  cguittfont
  ${IRRLICHT_LIBRARY}
  ${FREETYPE_LIBRARY}
  ${ZLIB_LIBRARIES}  # needed by freetype, repeated here for safety
)

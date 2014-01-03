# CGUITTFont authors, y u no include headers you use?
#   Do not add CGUITTFont.cpp to the line below.
#   xCGUITTFont.cpp is a wrapper file that includes
#   additional required headers.
add_library(cguittfont xCGUITTFont.cpp)

if(FREETYPE_PKGCONFIG_FOUND)
	set_target_properties(cguittfont
		PROPERTIES
		COMPILE_FLAGS "${FREETYPE_CFLAGS_STR}"
		LINK_FLAGS "${FREETYPE_LDFLAGS_STR}"
	)

	include_directories(
	  ${IRRLICHT_INCLUDE_DIR}
	)
else(FREETYPE_PKGCONFIG_FOUND)
	include_directories(
	  ${IRRLICHT_INCLUDE_DIR}
	  ${FREETYPE_INCLUDE_DIRS}
	)
endif(FREETYPE_PKGCONFIG_FOUND)

target_link_libraries(
	cguittfont
	${IRRLICHT_LIBRARY}
	${FREETYPE_LIBRARY}
	${ZLIB_LIBRARIES}  # needed by freetype, repeated here for safety
	)

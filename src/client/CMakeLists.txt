set(sound_SRCS  ${CMAKE_CURRENT_SOURCE_DIR}/sound.cpp)

if(USE_SOUND)
	set(sound_SRCS ${sound_SRCS}
		${CMAKE_CURRENT_SOURCE_DIR}/sound_openal.cpp)
	set(SOUND_INCLUDE_DIRS
		${OPENAL_INCLUDE_DIR}
		${VORBIS_INCLUDE_DIR}
		${OGG_INCLUDE_DIR}
		PARENT_SCOPE)
	set(SOUND_LIBRARIES
		${OPENAL_LIBRARY}
		${VORBIS_LIBRARIES}
		PARENT_SCOPE)
endif()

set(client_SRCS
	${sound_SRCS}
	${CMAKE_CURRENT_SOURCE_DIR}/meshgen/collector.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/anaglyph.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/core.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/factory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/interlaced.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/pageflip.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/plain.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/sidebyside.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/render/stereo.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/renderingengine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/clientlauncher.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/gameui.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/inputhandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/joystick_controller.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/hud.cpp
	PARENT_SCOPE
)

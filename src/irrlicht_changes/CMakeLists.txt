if (BUILD_CLIENT)
	set(client_irrlicht_changes_SRCS
		irrlicht_changes/irrUString.h
		irrlicht_changes/static_text.cpp
		irrlicht_changes/static_text.h
	)

	if (ENABLE_FREETYPE)
		set(client_irrlicht_changes_SRCS ${client_irrlicht_changes_SRCS}
			irrlicht_changes/CGUITTFont.cpp
			irrlicht_changes/CGUITTFont.h
		)
	endif()

	# CMake require us to set a local scope and then parent scope
	# Else the last set win in parent scope
	set(client_irrlicht_changes_SRCS ${client_irrlicht_changes_SRCS} PARENT_SCOPE)
endif()


# Locate Irrlicht or IrrlichtMt headers on system.

foreach(libname IN ITEMS IrrlichtMt Irrlicht)
	string(TOLOWER "${libname}" libname2)

	find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		DOC "Path to the directory with IrrlichtMt includes"
		PATHS
		/usr/local/include/${libname2}
		/usr/include/${libname2}
		/system/develop/headers/${libname2} #Haiku
		PATH_SUFFIXES "include/${libname2}"
	)

	if(IRRLICHT_INCLUDE_DIR)
		break()
	endif()
endforeach()

# Handholding for users
if(IRRLICHT_INCLUDE_DIR AND (NOT IS_DIRECTORY "${IRRLICHT_INCLUDE_DIR}" OR
	NOT EXISTS "${IRRLICHT_INCLUDE_DIR}/irrlicht.h"))
	message(WARNING "IRRLICHT_INCLUDE_DIR was set to ${IRRLICHT_INCLUDE_DIR} "
		"but irrlicht.h does not exist inside. The path will not be used.")
	unset(IRRLICHT_INCLUDE_DIR CACHE)
endif()

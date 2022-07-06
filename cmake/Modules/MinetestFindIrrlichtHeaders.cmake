# Locate IrrlichtMt headers on system.

find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
	DOC "Path to the directory with IrrlichtMt includes"
	PATHS
	/usr/local/include/irrlichtmt
	/usr/include/irrlichtmt
	/system/develop/headers/irrlichtmt #Haiku
	PATH_SUFFIXES "include/irrlichtmt"
)

# Handholding for users
if(IRRLICHT_INCLUDE_DIR AND (NOT IS_DIRECTORY "${IRRLICHT_INCLUDE_DIR}" OR
	NOT EXISTS "${IRRLICHT_INCLUDE_DIR}/irrlicht.h"))
	message(WARNING "IRRLICHT_INCLUDE_DIR was set to ${IRRLICHT_INCLUDE_DIR} "
		"but irrlicht.h does not exist inside. The path will not be used.")
	unset(IRRLICHT_INCLUDE_DIR CACHE)
endif()

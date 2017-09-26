set(common_network_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/connection.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/networkpacket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/serverpackethandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/serveropcodes.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/serverconnection.cpp
	PARENT_SCOPE
)

if (BUILD_CLIENT)
	set(client_network_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/clientopcodes.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/clientpackethandler.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/clientconnection.cpp
		PARENT_SCOPE
	)
endif()


set(common_network_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/connection.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/networkpacket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/packethandlers/server.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/serveropcodes.cpp
	PARENT_SCOPE
)

if (BUILD_CLIENT)
	set(client_network_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/clientopcodes.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/packethandlers/client.cpp
		PARENT_SCOPE
	)
endif(BUILD_CLIENT)
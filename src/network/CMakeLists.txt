set(common_network_SRCS
	network/address.cpp
	network/address.h
	network/connection.cpp
	network/connection.h
	network/connectionthreads.cpp
	network/connectionthreads.h
	network/networkexceptions.h
	network/networkpacket.cpp
	network/networkpacket.h
	network/networkprotocol.h
	network/peerhandler.h
	network/serverpackethandler.cpp
	network/serveropcodes.cpp
	network/serveropcodes.h
	network/socket.cpp
	network/socket.h
	PARENT_SCOPE
)

if (BUILD_CLIENT)
	set(client_network_SRCS
		network/clientopcodes.cpp
		network/clientopcodes.h
		network/clientpackethandler.cpp
		PARENT_SCOPE
	)
endif()

# Haiku networking support
if(HAIKU)
	set(PLATFORM_LIBS -lnetwork ${PLATFORM_LIBS})
endif()

set (UNITTEST_SRCS
	unittest/test.cpp
	unittest/test.h
	unittest/test_areastore.cpp
	unittest/test_collision.cpp
	unittest/test_compression.cpp
	unittest/test_connection.cpp
	unittest/test_filepath.cpp
	unittest/test_inventory.cpp
	unittest/test_map_settings_manager.cpp
	unittest/test_mapnode.cpp
	unittest/test_modchannels.cpp
	unittest/test_nodedef.cpp
	unittest/test_noderesolver.cpp
	unittest/test_noise.cpp
	unittest/test_objdef.cpp
	unittest/test_player.cpp
	unittest/test_profiler.cpp
	unittest/test_random.cpp
	unittest/test_schematic.cpp
	unittest/test_serialization.cpp
	unittest/test_settings.cpp
	unittest/test_socket.cpp
	unittest/test_threading.cpp
	unittest/test_utilities.cpp
	unittest/test_voxelalgorithms.cpp
	unittest/test_voxelmanipulator.cpp
	PARENT_SCOPE)

set (UNITTEST_CLIENT_SRCS
	unittest/test_keycode.cpp
	PARENT_SCOPE)

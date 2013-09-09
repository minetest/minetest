# Used by server and client
set(common_SCRIPT_LUA_API_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/l_base.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_craft.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_env.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_inventory.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_item.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_mapgen.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_nodemeta.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_nodetimer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_noise.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_object.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_particles.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_rollback.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_server.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_util.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_vmanip.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/l_settings.cpp
	PARENT_SCOPE)

# Used by client only
set(minetest_SCRIPT_LUA_API_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/l_mainmenu.cpp
	PARENT_SCOPE)

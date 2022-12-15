set(common_SCRIPT_SSCSM_SRCS
	PARENT_SCOPE)

set(client_SCRIPT_SSCSM_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/sscsm_controller.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/sscsm_message.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/sscsm_script_api.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/sscsm_script_process.cpp
	PARENT_SCOPE)

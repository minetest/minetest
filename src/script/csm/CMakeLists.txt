set(common_SCRIPT_CSM_SRCS
	PARENT_SCOPE)

set(client_SCRIPT_CSM_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/csm_controller.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/csm_gamedef.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/csm_scripting.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/csm_script_ipc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/csm_script_process.cpp
	PARENT_SCOPE)

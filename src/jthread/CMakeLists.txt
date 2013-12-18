if( UNIX )
	set(JTHREAD_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/pthread/jmutex.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pthread/jthread.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pthread/jsemaphore.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/pthread/jevent.cpp
		PARENT_SCOPE)
else( UNIX )
	set(JTHREAD_SRCS
		${CMAKE_CURRENT_SOURCE_DIR}/win32/jmutex.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/jthread.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/jsemaphore.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/win32/jevent.cpp
		PARENT_SCOPE)
endif( UNIX )

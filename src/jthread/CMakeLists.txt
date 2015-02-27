if(UNIX)
	set(THREAD_SYS_DIR pthread)
else()
	set(THREAD_SYS_DIR win32)
endif()

set(SRC_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/${THREAD_SYS_DIR})
set(JTHREAD_SRCS
	${SRC_PREFIX}/jmutex.cpp
	${SRC_PREFIX}/jthread.cpp
	${SRC_PREFIX}/jsemaphore.cpp
	${SRC_PREFIX}/jevent.cpp
	PARENT_SCOPE)


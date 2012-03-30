if( UNIX )
	set(jthread_SRCS pthread/jmutex.cpp pthread/jthread.cpp)
	set(jthread_platform_LIBS "")

	set(JTHREAD_CONFIG_WIN32THREADS "// Using pthread based threads")
	set(JTHREAD_CONFIG_JMUTEXCRITICALSECTION "")

else( UNIX )
	set(jthread_SRCS win32/jmutex.cpp win32/jthread.cpp)
	set(jthread_platform_LIBS "")

	set(JTHREAD_CONFIG_WIN32THREADS "#define JTHREAD_CONFIG_WIN32THREADS")
	set(JTHREAD_WIN32_CRITICALSECTION OFF CACHE BOOL "If set to false, use standard mutex. If set to true, use a critical section object.")
	if (JTHREAD_WIN32_CRITICALSECTION)
		set(JTHREAD_CONFIG_JMUTEXCRITICALSECTION "#define JTHREAD_CONFIG_JMUTEXCRITICALSECTION")
	else (JTHREAD_WIN32_CRITICALSECTION)
		set(JTHREAD_CONFIG_JMUTEXCRITICALSECTION "// Using standard Win32 mutex")
	endif (JTHREAD_WIN32_CRITICALSECTION)
endif( UNIX )

add_library(jthread ${jthread_SRCS})

target_link_libraries(
	jthread
	${jthread_platform_LIBS}
)

configure_file("${PROJECT_SOURCE_DIR}/jthread/jthreadconfig.h.in"
	       "${PROJECT_BINARY_DIR}/jthread/jthreadconfig.h")

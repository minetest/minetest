if( UNIX )
	set(jthread_SRCS pthread/jmutex.cpp pthread/jthread.cpp)
	set(jthread_platform_LIBS "")
else( UNIX )
	set(jthread_SRCS win32/jmutex.cpp win32/jthread.cpp)
	set(jthread_platform_LIBS "")
endif( UNIX )

add_library(jthread ${jthread_SRCS})

target_link_libraries(
	jthread
	${jthread_platform_LIBS}
)



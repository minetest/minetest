if( UNIX )
	set(json_SRCS jsoncpp.cpp)
	set(json_platform_LIBS "")
else( UNIX )
	set(json_SRCS jsoncpp.cpp)
	set(json_platform_LIBS "")
endif( UNIX )

add_library(jsoncpp ${json_SRCS})

target_link_libraries(
	jsoncpp
	${json_platform_LIBS}
)

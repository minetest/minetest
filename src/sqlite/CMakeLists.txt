if( UNIX )
	set(sqlite3_SRCS sqlite3.c)
	set(sqlite3_platform_LIBS "")
else( UNIX )
	set(sqlite3_SRCS sqlite3.c)
	set(sqlite3_platform_LIBS "")
endif( UNIX )

add_library(sqlite3 ${sqlite3_SRCS})

target_link_libraries(
	sqlite3
	${sqlite3_platform_LIBS}
)



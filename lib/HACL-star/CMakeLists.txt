set(HACL_MAIN_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dist/msvc-compatible CACHE STRING "Main HACL* source path")
set(HACL_KARAMEL_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dist/karamel CACHE STRING "Karamel source path")

set(HACL_C_SRC
	${HACL_MAIN_SRC_PATH}/Lib_Memzero0.c
	${HACL_MAIN_SRC_PATH}/Hacl_AEAD_Chacha20Poly1305.c
	${HACL_MAIN_SRC_PATH}/Hacl_Chacha20.c
	${HACL_MAIN_SRC_PATH}/Hacl_Curve25519_51.c
	${HACL_MAIN_SRC_PATH}/Hacl_Ed25519.c
	${HACL_MAIN_SRC_PATH}/Hacl_HKDF.c
	${HACL_MAIN_SRC_PATH}/Hacl_HMAC.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_Base.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_Blake2b.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_Blake2s.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_SHA1.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_SHA2.c
	${HACL_MAIN_SRC_PATH}/Hacl_MAC_Poly1305.c
	${HACL_MAIN_SRC_PATH}/Vale.c
)

set(HACL_ABI "unknown")
set(HACL_PLATFORM "unknown")
set(TARGET_ARCHITECTURE 0)
set(HACL_HAS_VALE FALSE)

add_library(HACL-star STATIC
	${HACL_C_SRC})

if (ANDROID)
	target_compile_definitions(HACL-star PRIVATE LINUX_NO_EXPLICIT_BZERO)
endif()

set_source_files_properties(${HACL_SRC} PROPERTIES LANGUAGE C)

target_compile_definitions(HACL-star PRIVATE TARGET_ARCHITECTURE=${TARGET_ARCHITECTURE})
target_include_directories(HACL-star PUBLIC ${HACL_MAIN_SRC_PATH})
target_include_directories(HACL-star PUBLIC "${HACL_KARAMEL_SRC_PATH}/include")
target_include_directories(HACL-star PUBLIC "${HACL_KARAMEL_SRC_PATH}/krmllib/dist/minimal")
target_include_directories(HACL-star PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)

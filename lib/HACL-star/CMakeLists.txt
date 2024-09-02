set(HACL_MAIN_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dist/msvc-compatible CACHE STRING "Main HACL* source path")
set(HACL_KARAMEL_SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/dist/karamel CACHE STRING "Karamel source path")

set(HACL_SRC
	${HACL_MAIN_SRC_PATH}/Lib_Memzero0.c
	${HACL_MAIN_SRC_PATH}/Lib_PrintBuffer.c 
	${HACL_MAIN_SRC_PATH}/Lib_RandomBuffer_System.c
	${HACL_MAIN_SRC_PATH}/EverCrypt_AEAD.c
	${HACL_MAIN_SRC_PATH}/EverCrypt_AutoConfig2.c
	${HACL_MAIN_SRC_PATH}/EverCrypt_Chacha20Poly1305.c
	${HACL_MAIN_SRC_PATH}/EverCrypt_Curve25519.c
	${HACL_MAIN_SRC_PATH}/EverCrypt_HKDF.c 
	${HACL_MAIN_SRC_PATH}/EverCrypt_HMAC.c 
	${HACL_MAIN_SRC_PATH}/EverCrypt_Hash.c
	${HACL_MAIN_SRC_PATH}/Hacl_AEAD_Chacha20Poly1305.c
	${HACL_MAIN_SRC_PATH}/Hacl_Chacha20.c
	${HACL_MAIN_SRC_PATH}/Hacl_Curve25519_51.c
	${HACL_MAIN_SRC_PATH}/Hacl_Curve25519_64.c
	${HACL_MAIN_SRC_PATH}/Hacl_HKDF.c 
	${HACL_MAIN_SRC_PATH}/Hacl_HMAC.c 
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_Base.c 
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_Blake2b.c
	#${HACL_MAIN_SRC_PATH}/Hacl_Hash_Blake2b_Simd256.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_Blake2s.c
	#${HACL_MAIN_SRC_PATH}/Hacl_Hash_Blake2s_Simd128.c
	#${HACL_MAIN_SRC_PATH}/Hacl_Hash_MD5.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_SHA1.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_SHA2.c
	${HACL_MAIN_SRC_PATH}/Hacl_Hash_SHA3.c
	#${HACL_MAIN_SRC_PATH}/Hacl_Hash_SHA3_Simd256.c
	${HACL_MAIN_SRC_PATH}/Vale.c
)

set(HACL_ABI "unknown")
set(HACL_PLATFORM "unknown")
set(HACL_HAS_VALE FALSE)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
	set(HACL_PLATFORM "x86_64")
	set(HACL_HAS_VALE TRUE)
	if (MSVC)
		set(HACL_ABI "msvc")
	elseif(WIN32) # MinGW
		set(HACL_ABI "mingw")
	elseif(APPLE)
		set(HACL_ABI "darwin")
	elseif(UNIX)
		set(HACL_ABI "linux")
	else()
		set(HACL_HAS_VALE FALSE)
	endif()
endif()


set(HACL_TARGET ${HACL_PLATFORM}-${HACL_ABI})

set(HACL_ASM_SRC,
	${HACL_MAIN_SRC_PATH}/sha256-${HACL_TARGET}.asm,
	${HACL_MAIN_SRC_PATH}/curve25519-${HACL_TARGET}.asm,
	${HACL_MAIN_SRC_PATH}/cpuid-${HACL_TARGET}.asm,
	${HACL_MAIN_SRC_PATH}/aesgcm-${HACL_TARGET}.asm
)

add_library(HACL-star STATIC 
	${HACL_SRC}
	$<$<BOOL:${HACL_HAS_VALE}>:${HACL_ASM_SRC}>)

set_source_files_properties(${HACL_SRC} PROPERTIES LANGUAGE C)
if (HACL_HAS_VALE)
	set_source_files_properties(${HACL_ASM_SRC} PROPERTIES LANGUAGE ASM)
	target_compile_definitions(HACL-star PRIVATE HACL_CAN_COMPILE_VALE)
endif()

target_include_directories(HACL-star PUBLIC ${HACL_MAIN_SRC_PATH})
target_include_directories(HACL-star PUBLIC "${HACL_KARAMEL_SRC_PATH}/include")
target_include_directories(HACL-star PUBLIC "${HACL_KARAMEL_SRC_PATH}/krmllib/dist/minimal")
target_include_directories(HACL-star PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/config)

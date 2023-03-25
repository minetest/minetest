add_library(sha256 STATIC sha256.c)

target_include_directories(sha256 INTERFACE .)

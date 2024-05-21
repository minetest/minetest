add_executable(tests
	testCGLTFMeshFileLoader.cpp
	"${PROJECT_SOURCE_DIR}/source/Irrlicht/CReadFile.cpp"
)

target_compile_options(tests
	PRIVATE
		"$<$<CXX_COMPILER_ID:GNU>:-Wall>"
)

target_include_directories(tests
	PRIVATE
		# For CReadFile
		"${PROJECT_SOURCE_DIR}/source/Irrlicht"
)

target_link_libraries(tests
	PRIVATE
		Catch2::Catch
		IrrlichtMt::IrrlichtMt
)

add_test(
	NAME tests
	COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests"
	WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

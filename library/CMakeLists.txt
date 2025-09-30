
add_subdirectory(source)

target_include_directories(MPIPCL PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	#$<INSTALL_INTERFACE:src/core/include>
)
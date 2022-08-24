cmake_minimum_required (VERSION 3.10 FATAL_ERROR)

# set ExaMPI Project
project(MPIPCL VERSION 0.1 LANGUAGES C)

# set compiler (not good practice)
set(CMAKE_C_COMPILER "mpicc")

# determine build type
if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting build type to DEBUG.")
	set(CMAKE_BUILD_TYPE "DEBUG")
endif()


##### available options #####
option(BUILD_STATIC_LIBS "Build MPIPCL static library." ON)
option(BUILD_SHARED_LIBS "Build MPIPCL shared library." OFF)

##### available targets #####
add_library(mpipcl "")

##### target exampi #####
set_target_properties(mpipcl PROPERTIES LINKER_LANGUAGE C)
set_target_properties(mpipcl PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(mpipcl PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(mpipcl PROPERTIES PUBLIC_HEADER include/mpipcl.h)
set_target_properties(mpipcl PROPERTIES C_STANDARD 99)

# traverse subdirectories
add_subdirectory(src)
add_subdirectory(include)

# set compile options
if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
	# add additional debug flags for DEBUG
	target_compile_options(mpipcl PRIVATE -Wall -Wextra -Wpedantic)

	target_compile_options(mpipcl PRIVATE -Wno-unused-parameter)
	target_compile_options(mpipcl PRIVATE -Wno-unused-function)
	target_compile_options(mpipcl PRIVATE -Wno-unused-variable)
	target_compile_options(mpipcl PRIVATE -Wno-unused-but-set-variable)
endif()

if(CMAKE_BUILD_TYPE MATCHES "RELEASE")
	# add additional performance optimization for RELEASE
	target_compile_options(mpipcl PRIVATE -funroll-loops)
	target_compile_options(mpipcl PRIVATE -ggdb)
endif()


##### install target mpipcl #####
install(
	TARGETS mpipcl
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

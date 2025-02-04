cmake_minimum_required (VERSION 3.17 FATAL_ERROR)
project(MPIPCL VERSION 1.4.0 LANGUAGES C)

##### Available options #####
option(BUILD_STATIC_LIBS "Build MPIPCL static library." OFF)
option(BUILD_SHARED_LIBS "Build MPIPCL shared library." ON)
option(WITH_DEBUG "Turn debug statements on or off." OFF)
option(BUILD_EXAMPLES "Turn on building of MPIPCL Examples." OFF)
option(UNIQUE_NAMES "Changes the types and names of functions to MPIP instead of MPIX." OFF)

##### Create main project target: mpipcl #####
add_library(mpipcl "")

set_target_properties(mpipcl PROPERTIES LINKER_LANGUAGE C)
set_target_properties(mpipcl PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(mpipcl PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(mpipcl PROPERTIES PUBLIC_HEADER include/mpipcl.h)
set_target_properties(mpipcl PROPERTIES C_STANDARD 99)

# set compile options for project

# determine build type
if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting build type to RELEASE.")
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
	# add additional debug flags for DEBUG
	target_compile_options(mpipcl PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(mpipcl PRIVATE -Wno-unused-parameter)
	target_compile_options(mpipcl PRIVATE -ggdb)
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG" OR WITH_DEBUG)
	add_definitions(-DWITH_DEBUG)
endif()

if(CMAKE_BUILD_TYPE MATCHES "RELEASE")
	# add additional performance optimization for RELEASE
	target_compile_options(mpipcl PRIVATE -funroll-loops)
endif()

if(UNIQUE_NAMES)
	message(STATUS "Using MPIP instead of MPIX for APIs")
	add_definitions( -DMPIPCL_UNIQUE_NAMES )
endif()

# Link MPI to project
find_package(MPI REQUIRED)
target_link_libraries(mpipcl MPI::MPI_C)
find_package(Threads REQUIRED)
target_link_libraries(mpipcl Threads::Threads)

##### traverse subdirectories #####
add_subdirectory(src)
add_subdirectory(include)
if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

##### install mpipcl #####
install(
	TARGETS mpipcl
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)

cmake_minimum_required (VERSION 3.17 FATAL_ERROR)
project(MPIPCL VERSION 1.4.0 LANGUAGES C)
include(CMakeDependentOption)

##### Available options #####
option(BUILD_SHARED_LIBS "Build MPIPCL shared library." ON)
option(WITH_DEBUG "Turn debug statements on or off." OFF)
option(BUILD_EXAMPLES "Turn on building of MPIPCL Examples." OFF)
cmake_dependent_option(EXAMPLES_TO_BIN "Create examples in <install>/bin. requires BUILD_EXAMPLES" OFF "BUILD_EXAMPLES" OFF)
option(UNIQUE_NAMES "Changes the types and names of functions to MPIP instead of MPIX." OFF)

## Check for incompatible options
if(UNIQUE_NAMES AND BUILD_EXAMPLES)
	message( FATAL_ERROR "No examples currently exist using the MPIP version of the API")
endif()

##### Create main project target: MPIPCL #####
add_library(MPIPCL "")

## Basic target properties
set_target_properties(MPIPCL PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(MPIPCL PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(MPIPCL PROPERTIES PUBLIC_HEADER include/mpipcl.h)
set_target_properties(MPIPCL PROPERTIES LINKER_LANGUAGE C)
set_target_properties(MPIPCL PROPERTIES C_STANDARD 11)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "No CMAKE_BUILD_TYPE specified, setting build type to RELEASE.")
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

# Convert to lower case string to ignore case
string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE} -> ${build_type}")

# Build type options for debug
if(build_type STREQUAL "debug")
	# add additional debug flags for DEBUG
	target_compile_options(MPIPCL PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(MPIPCL PRIVATE -Wno-unused-parameter)
	set(WITH_DEBUG ON)
elseif(build_type STREQUAL "release")
	# add additional performance optimization for RELEASE
	target_compile_options(MPIPCL PRIVATE -funroll-loops)
endif()

if(WITH_DEBUG)
	message(STATUS "Enabling debugging print outs")
	add_definitions(-DWITH_DEBUG)
endif()

if(UNIQUE_NAMES)
	message(STATUS "Using MPIP instead of MPIX for APIs")
	# PUBLIC target for both this library and anyone who uses it
	target_compile_definitions (MPIPCL PUBLIC MPIPCL_UNIQUE_NAMES=1 )
endif()

# Find extra libraries to link to
find_package(MPI REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(MPIPCL PUBLIC MPI::MPI_C Threads::Threads)

##### Traverse subdirectories #####
add_subdirectory(src)
add_subdirectory(include)
if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

##### install mpipcl #####
install(TARGETS MPIPCL
	    EXPORT MPIPCLTargets)

##### Code for CMake Packaging
include(CMakePackageConfigHelpers)

# Create the configuration file
configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/mpipcl-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	INSTALL_DESTINATION share/${PROJECT_NAME}
)

## Version file
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config-version.cmake
	COMPATIBILITY SameMinorVersion
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config-version.cmake
	${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
	DESTINATION share/${PROJECT_NAME}
)

# Install everything
install(EXPORT MPIPCLTargets
	NAMESPACE MPIAdvance::
	DESTINATION share/${PROJECT_NAME}
)

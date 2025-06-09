cmake_minimum_required (VERSION 3.17 FATAL_ERROR)
project(MPIPCL VERSION 1.4.0 LANGUAGES C)
include(CMakeDependentOption)

##### Available options #####
option(BUILD_SHARED_LIBS "Build MPIPCL shared library." ON)
option(WITH_DEBUG "Turn debug statements on or off." OFF)
option(BUILD_EXAMPLES "Turn on building of MPIPCL Examples." OFF)
cmake_dependent_option(EXAMPLES_TO_BIN "Create examples in <install>/bin. requires BUILD_EXAMPLES" OFF "BUILD_EXAMPLES" OFF)
option(UNIQUE_NAMES "Changes the types and names of functions to MPIP instead of MPIX." OFF)

##### Create main project target: MPIPCL #####
add_library(MPIPCL "")

set_target_properties(MPIPCL PROPERTIES LINKER_LANGUAGE C)
set_target_properties(MPIPCL PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(MPIPCL PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})
set_target_properties(MPIPCL PROPERTIES PUBLIC_HEADER include/mpipcl.h)
set_target_properties(MPIPCL PROPERTIES C_STANDARD 11)

# set compile options for project

# determine build type
if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Setting build type to RELEASE.")
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
	# add additional debug flags for DEBUG
	target_compile_options(MPIPCL PRIVATE -Wall -Wextra -Wpedantic)
	target_compile_options(MPIPCL PRIVATE -Wno-unused-parameter)
	target_compile_options(MPIPCL PRIVATE -ggdb)
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG" OR WITH_DEBUG)
	add_definitions(-DWITH_DEBUG)
endif()

if(CMAKE_BUILD_TYPE MATCHES "RELEASE")
	# add additional performance optimization for RELEASE
	target_compile_options(MPIPCL PRIVATE -funroll-loops)
endif()

if(UNIQUE_NAMES)
	message(STATUS "Using MPIP instead of MPIX for APIs")
	# PUBLIC target for both this library and anyone who uses it
	target_compile_definitions (MPIPCL PUBLIC MPIPCL_UNIQUE_NAMES=1 )
endif()

# Link MPI to project
find_package(MPI REQUIRED)
target_link_libraries(MPIPCL MPI::MPI_C)
find_package(Threads REQUIRED)
target_link_libraries(MPIPCL Threads::Threads)

##### traverse subdirectories #####
add_subdirectory(src)
add_subdirectory(include)
if(BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

##### install mpipcl #####
install(TARGETS MPIPCL
	EXPORT MPIPCLTargets
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include
)


##### Code for CMake Packaging
include(CMakePackageConfigHelpers)

## Version file
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/mpipcl-config-version.cmake
	COMPATIBILITY AnyNewerVersion
)

# Create the configuration file
configure_package_config_file(
	${CMAKE_CURRENT_SOURCE_DIR}/cmake/mpipcl-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/mpipcl-config.cmake
	INSTALL_DESTINATION share/mpipcl
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/mpipcl-config-version.cmake
	${CMAKE_CURRENT_BINARY_DIR}/mpipcl-config.cmake
	DESTINATION share/mpipcl
)

# Install everything
install(EXPORT MPIPCLTargets
	NAMESPACE MPIPCL::
	DESTINATION share/mpipcl
)

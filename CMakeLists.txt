cmake_minimum_required (VERSION 3.17 FATAL_ERROR)
project(MPIPCL VERSION 1.4.0 LANGUAGES C CXX)
include(CMakeDependentOption)

##### Available options #####
option(BUILD_SHARED_LIBS "Build MPIPCL shared library." ON)
option(BUILD_EXAMPLES "Turn on building of MPIPCL Examples." OFF)
cmake_dependent_option(EXAMPLES_TO_BIN "Create examples in <install>/bin. requires BUILD_EXAMPLES" OFF "BUILD_EXAMPLES" OFF)

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

if(CMAKE_BUILD_TYPE MATCHES "RELEASE")
	# add additional performance optimization for RELEASE
	target_compile_options(MPIPCL PRIVATE -funroll-loops)
endif()

# Link MPI to project
find_package(MPI REQUIRED)
find_package(Threads REQUIRED)
find_package(MPIAdvanceBase REQUIRED)

target_link_libraries(MPIPCL PUBLIC MPI::MPI_CXX Threads::Threads MPIAdvance::Base)

##### traverse subdirectories #####
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
	NAMESPACE MPIAdvance::
	DESTINATION share/mpipcl
)

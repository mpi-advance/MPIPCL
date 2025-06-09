set(mpipcl_test_execs "")
set(mpipcl_test_cxx_execs "")
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

add_executable(MPIPCL_Test_1 mpipcltest1.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_1)

add_executable(MPIPCL_Test_2 mpipcltest2.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_2)

add_executable(MPIPCL_Test_3 mpipcltest3.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_3)

add_executable(MPIPCL_Test_4 mpipcltest4.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_4)

add_executable(MPIPCL_Test_5 mpipcltest5.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_5)

add_executable(MPIPCL_Test_6 mpipcltest6.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_6)

add_executable(MPIPCL_Test_7 mpipcltest7.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_7)

add_executable(MPIPCL_Test_8 mpipcltest8.c)
list(APPEND mpipcl_test_execs MPIPCL_Test_8)

# Turn on C++ for this test
enable_language(CXX)
# Re-find MPI because we need the MPI::MPI_CXX (linking) target
# This not C++ MPI bindings, but the C++ flags to compile with MPIC++
find_package(MPI REQUIRED)
add_executable(MPIPCL_Test_9 mpipcltest9.cpp)
list(APPEND mpipcl_test_cxx_execs MPIPCL_Test_9)

foreach(exec_target ${mpipcl_test_execs})
	if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
		target_compile_options(${exec_target} PRIVATE -Wall -Wextra -Wpedantic)
	endif()
	#message (STATUS "RPATH = ${CMAKE_INSTALL_RPATH}")
	target_include_directories(${exec_target} PUBLIC ${PROJECT_SOURCE_DIR}/include)
	target_link_libraries(${exec_target} PUBLIC MPI::MPI_C)
    	target_link_libraries(${exec_target} PUBLIC MPIPCL)
	target_link_libraries(${exec_target} PUBLIC Threads::Threads)
	if(EXAMPLES_TO_BIN)
		install(TARGETS ${exec_target})
	endif()
	
endforeach()

foreach(exec_target ${mpipcl_test_cxx_execs})
	if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
		target_compile_options(${exec_target} PRIVATE -Wall -Wextra -Wpedantic)
	endif()
	target_include_directories(${exec_target} PUBLIC ${PROJECT_SOURCE_DIR}/include)
	target_link_libraries(${exec_target} PUBLIC MPI::MPI_CXX)
	target_link_libraries(${exec_target} PUBLIC MPIPCL)
	target_link_libraries(${exec_target} PUBLIC Threads::Threads)
	if(EXAMPLES_TO_BIN)
		install(TARGETS ${exec_target})
	endif()

endforeach()

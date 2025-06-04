set(mpipcl_test_execs "")
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

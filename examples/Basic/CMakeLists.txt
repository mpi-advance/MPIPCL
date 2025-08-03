set(mpipcl_test_execs "")
set(mpipcl_test_cxx_execs "")

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

add_executable(MPIPCL_Test_9 mpipcltest9.cpp)
list(APPEND mpipcl_test_cxx_execs MPIPCL_Test_9)

foreach(exec_target ${mpipcl_test_execs})
	if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
		target_compile_options(${exec_target} PRIVATE -Wall -Wextra -Wpedantic)
	endif()
	target_include_directories(${exec_target} PUBLIC ${PROJECT_SOURCE_DIR}/include)
	target_link_libraries(${exec_target} PUBLIC MPI::MPI_C MPIPCL Threads::Threads MPIAdvance::Base)
	if(EXAMPLES_TO_BIN)
		install(TARGETS ${exec_target})
	endif()	
endforeach()

foreach(exec_target ${mpipcl_test_cxx_execs})
	if(CMAKE_BUILD_TYPE MATCHES "DEBUG")
		target_compile_options(${exec_target} PRIVATE -Wall -Wextra -Wpedantic)
	endif()
	target_include_directories(${exec_target} PUBLIC ${PROJECT_SOURCE_DIR}/include)
	target_link_libraries(${exec_target} PUBLIC MPI::MPI_CXX MPIPCL Threads::Threads MPIAdvance::Base)
	if(EXAMPLES_TO_BIN)
		install(TARGETS ${exec_target})
	endif()
endforeach()

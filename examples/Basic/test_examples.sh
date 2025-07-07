#!/bin/bash

declare -a FAILURES=()

mpirun -n 2 ./MPIPCL_Test_1 10 1000   >/dev/null 2>&1 || FAILURES+=("1") 
mpirun -n 2 ./MPIPCL_Test_2 10 1000   >/dev/null 2>&1 || FAILURES+=("2")
mpirun -n 5 ./MPIPCL_Test_3 10 1000   >/dev/null 2>&1 || FAILURES+=("3")
mpirun -n 5 ./MPIPCL_Test_4 10 1000   >/dev/null 2>&1 || FAILURES+=("4")
mpirun -n 2 ./MPIPCL_Test_5 10 1000   >/dev/null 2>&1 || FAILURES+=("5")
mpirun -n 2 ./MPIPCL_Test_6 10 1000 0 >/dev/null 2>&1 || FAILURES+=("6-0")
mpirun -n 2 ./MPIPCL_Test_6 10 1000 1 >/dev/null 2>&1 || FAILURES+=("6-1")
mpirun -n 2 ./MPIPCL_Test_6 10 1000 2 >/dev/null 2>&1 || FAILURES+=("6-2")
mpirun -n 2 ./MPIPCL_Test_6 10 1000 3 >/dev/null 2>&1 || FAILURES+=("6-3")
mpirun -n 2 ./MPIPCL_Test_7 10 1000   >/dev/null 2>&1 || FAILURES+=("7")
mpirun -n 2 ./MPIPCL_Test_8 10 1000   >/dev/null 2>&1 || FAILURES+=("8")
mpirun -n 2 ./MPIPCL_Test_9 10 1000   >/dev/null 2>&1 || FAILURES+=("9")


echo ${#FAILURES[@]} "FAILURES Detected" 

if [ "${#FAILURES[@]}" -ne 0 ]; then  
	echo "Failures on tests"	
	echo ${FAILURES[@]}
fi
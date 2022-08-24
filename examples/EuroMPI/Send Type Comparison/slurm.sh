#!/bin/bash

#SBATCH --nodes=2

#SBATCH --time=00:05:00

mpirun -n 2 ./a.exe  > send.csv
mpirun -n 2 ./b.exe  >> send.csv
mpirun -n 2 ./c.exe  >> send.csv
echo 'version1'>> send.csv
mpirun -n 2 ./d1.exe >> send.csv
echo 'version2' >> send.csv
mpirun -n 2 ./d2.exe >> send.csv
echo 'version3' >> send.csv
mpirun -n 2 ./d3.exe >> send.csv

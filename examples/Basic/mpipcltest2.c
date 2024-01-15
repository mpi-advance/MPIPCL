/* Sample program to test the partitioned communication API.
 * This version uses threads on the receive side also.
 *
 * To compile: 
 *    mpicc -O -Wall -fopenmp -o mpipcltest2 mpipcltest2.c mpipcl.c
 * To run:
 *    mpirun -np 2 ./mpipcltest2 <npartitions> <bufsize>
 *    NOTE: bufsize % npartitions == 0
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include "mpipcl.h"

int main(int argc, char *argv[]) {
  int rank, size, nparts, bufsize, count, tag = 0xbad;
  int i, j, provided;
  double *buf, sum;
  MPIX_Request req;
  MPI_Status status;

  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
  assert(provided == MPI_THREAD_SERIALIZED);
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  assert(size == 2);

  if (argc != 3) {
    printf("Usage: %s <#partitions> <bufsize>\n", argv[0]);
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  nparts = atoi(argv[1]);
  bufsize = atoi(argv[2]);
  count = bufsize/nparts;
  
  if ((size != 2) || (bufsize % nparts != 0)) {
    printf("comm size must be 2 and bufsize must be divisible by nparts\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  }
  
  buf = malloc(sizeof(double) * bufsize);

  if (rank == 0) { /* sender */
    MPIX_Psend_init(buf, nparts, count, MPI_DOUBLE, 1, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req);
    MPIX_Start(&req);

    #pragma omp parallel for private(j) shared(buf,req) num_threads(nparts)
    for (i = 0; i < nparts; i++) {
       /* initialize part of buffer in each thread */
       for (j = 0; j < count; j++) 
          buf[j + i*count] = j + i*count + 1.0;

       /* indicate buffer is ready */
       MPIX_Pready(i, &req);
    }
    MPIX_Wait(&req, &status);

  } else {         /* receiver */
    MPIX_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req);
    MPIX_Start(&req);

    #pragma omp parallel for shared(buf,req,sum) num_threads(nparts)
      for (i = 0; i < nparts; i++) {
        int j, flag = 0; 
        double mysum = 0.0;
        while (!flag) {
            /* check if partition has been received */
            MPIX_Parrived(&req, i, &flag); 

            if (flag) {
              /* compute the partial sum of the values received */
              for (j = 0, mysum = 0.0; j < count; j++)
                mysum += buf[j + i*count];

              /* update global sum */
              #pragma omp critical 
              sum += mysum;
            }
        }
      }

    MPIX_Wait(&req, MPI_STATUS_IGNORE);
    printf("#partitions = %d bufsize = %d count = %d sum = %f (%f)\n", 
           nparts, bufsize, count, sum, ((double)bufsize*(bufsize+1))/2.0);
  }

  MPIX_Request_free(&req);
  free(buf);
  MPI_Finalize();

  return 0;
}

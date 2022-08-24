/* Sample program to test the partitioned communication API.
 * This program tests MPIX_Startall, MPIX_Pready, and MPIX_Waitall functions.
 * This version uses threads on both send-side and receive-side.
 *
 * To compile: 
 *    mpicc -O -Wall -fopenmp -o mpipcltest4 mpipcltest4.c mpipcl.c
 * To run:
 *    mpirun -np 5 ./mpipcltest4 <npartitions> <bufsize>
 *    NOTE: bufsize % npartitions == 0
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include "mpipcl.h"

#define NNEIGHBORS 4

int main(int argc, char *argv[]) {
  int rank, size, nparts, bufsize, count, tag = 0xbad;
  int i, j, provided;
  double *buf, sum;
  MPIX_Request req[NNEIGHBORS];

  MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
  assert(provided == MPI_THREAD_SERIALIZED);
  
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 3) {
    printf("Usage: %s <#partitions> <bufsize>\n", argv[0]);
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  nparts = atoi(argv[1]);
  bufsize = atoi(argv[2]);
  count = bufsize/nparts;

  if ((size != 5) || (bufsize % nparts != 0)) {
    printf("comm size must be 5 and bufsize must be divisible by nparts\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  }
  assert((buf = malloc(sizeof(double) * bufsize)) != NULL);

  if (rank == 0) { /* sender */
    for (i = 1; i <= NNEIGHBORS; i++) 
       assert(MPIX_Psend_init(buf, nparts, count, MPI_DOUBLE, i, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req[i-1]) == MPI_SUCCESS);
    assert(MPIX_Startall(NNEIGHBORS, req) == MPI_SUCCESS);
    printf("[%d]: nparts = %d bufsize = %d count = %d size = %d\n",
           rank, nparts, bufsize, count, size);

    #pragma omp parallel for private(j) shared(buf,req) num_threads(nparts)
    for (i = 0; i < nparts; i++) {
       /* initialize part of buffer in each thread */
       for (j = 0; j < count; j++) 
          buf[j + i*count] = j + i*count + 1.0;

       /* indicate buffer is ready for all sends */
       for (j = 0; j < NNEIGHBORS; j++)
	 assert(MPIX_Pready(i, &req[j]) == MPI_SUCCESS);
    }

    assert(MPIX_Waitall(NNEIGHBORS, req, MPI_STATUSES_IGNORE) == MPI_SUCCESS); 
    for (i = 0; i < NNEIGHBORS; i++) 
       assert(MPIX_Request_free(&req[i]) == MPI_SUCCESS);
  } else {         /* receiver */
    MPIX_Request req;

    assert(MPIX_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req) == MPI_SUCCESS);
    assert(MPIX_Start(&req) == MPI_SUCCESS);

    #pragma omp parallel for shared(buf,req,sum) num_threads(nparts)
    for (i = 0; i < nparts; i++) {
       int j, flag = 0, testflag; 
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
          } else {
            /* do some other work */
            MPIX_Test(&req, &testflag, MPI_STATUS_IGNORE);
            /* do some other work based on testflag */
          }
       }
    }
    assert(MPIX_Wait(&req, MPI_STATUS_IGNORE) == MPI_SUCCESS);
    printf("[%d]: #partitions = %d bufsize = %d count = %d sum = %f (%f)\n", 
           rank, nparts, bufsize, count, sum, ((double)bufsize*(bufsize+1))/2.0);
    assert(MPIX_Request_free(&req) == MPI_SUCCESS);
  }

  free(buf);
  MPI_Finalize();

  return 0;
}

/* Sample program to test the partitioned communication API.
 * This program tests MPIX_Startall, MPIX_Pready_range, and 
 * MPIX_Waitall functions.
 *
 * To compile: 
 *    mpicc -O -Wall -fopenmp -o mpipcltest3 mpipcltest3.c mpipcl.c
 * To run:
 *    mpirun -np 5 ./mpipcltest3 <npartitions> <bufsize>
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
  int i, provided;
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

    /* initialize buffer */
    for (i = 0; i < bufsize; i++) 
       buf[i] = i + 1.0;

    /* indicate buffer is ready */
    for (i = 0; i < NNEIGHBORS; i++) 
       assert(MPIX_Pready_range(0, nparts-1, &req[i]) == MPI_SUCCESS);

    assert(MPIX_Waitall(NNEIGHBORS, req, MPI_STATUSES_IGNORE) == MPI_SUCCESS); 
    for (i = 0; i < NNEIGHBORS; i++) 
       assert(MPIX_Request_free(&req[i]) == MPI_SUCCESS);
  } else {         /* receiver */

    assert(MPIX_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req[0]) == MPI_SUCCESS);
    assert(MPIX_Start(&req[0]) == MPI_SUCCESS);
    assert(MPIX_Wait(&req[0], MPI_STATUS_IGNORE) == MPI_SUCCESS);

    /* compute the sum of the values received */
    for (i = 0, sum = 0.0; i < bufsize; i++)
       sum += buf[i];

    assert(MPIX_Request_free(&req[0]) == MPI_SUCCESS);
    printf("[%d]: #partitions = %d bufsize = %d count = %d sum = %f (%f)\n", 
           rank, nparts, bufsize, count, sum, ((double)bufsize*(bufsize+1))/2.0);
  }

  free(buf);
  MPI_Finalize();

  return 0;
}

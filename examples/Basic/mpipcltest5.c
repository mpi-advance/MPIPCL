/* Sample program to test the partitioned communication API.

 * This program tests MPIX_Waitsome and MPIX_Testsome
 * MPIX_Waitall functions.
 *
 * To compile:
 *    mpicc -O -Wall -fopenmp -o mpipcltest5 mpipcltest5.c mpipcl.c
 * To run:
 *    mpirun -np 5 ./mpipcltest5 <npartitions> <bufsize>

 *    NOTE: bufsize % npartitions == 0
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include "mpipcl.h"

//#define NUMREQ 2

int main(int argc, char *argv[]) {
  int NUMREQ = 2;
  int rank, size, nparts, bufsize, count; //tag = 0xbad;
  int i=0, provided;
  double *buf, sum;
  MPIX_Request req[NUMREQ];

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


  if ((size != 2) || (bufsize % nparts != 0)) {
    printf("comm size must be 2 and bufsize must be divisible by nparts\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  assert((buf = malloc(sizeof(double) * bufsize)) != NULL);

  if (rank == 0) { /* sender */

	//make Num requests
	for(int i = 0; i<NUMREQ; i++)
		assert(MPIX_Psend_init(buf, nparts, count, MPI_DOUBLE, 1, i, MPI_COMM_WORLD, MPI_INFO_NULL, &req[i]) == MPI_SUCCESS);

    	/* initialize buffer */
    	for (i = 0; i < bufsize; i++)
       	buf[i] = i + 1.0;


    	//start each request in queue seperately
    	for(int j = 0; j<NUMREQ; j++){
    	//start request
    	assert(MPIX_Start(&req[j]) == MPI_SUCCESS);

    	/* indicate buffer is ready */
    	assert(MPIX_Pready_range(0, nparts-1, &req[j]) == MPI_SUCCESS);

    	/* wait for first request to complete before starting next */
    	assert(MPIX_Wait(&req[j], MPI_STATUSES_IGNORE) == MPI_SUCCESS);
    }

    /*Clean up*/
    for (i = 0; i < NUMREQ; i++)
       assert(MPIX_Request_free(&req[i]) == MPI_SUCCESS);




  } else if(rank == 1){         /* receiver */

    //clear buffers
    for(int i = 0; i<bufsize;i++)
    	buf[i] = 0.0;

    //make requests
    for(int i=0;i<NUMREQ; i++)
   	 assert(MPIX_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, i, MPI_COMM_WORLD, MPI_INFO_NULL, &req[i]) == MPI_SUCCESS);

    //start all but last request
    for(int k=0; k<NUMREQ-1; k++)
   	 assert(MPIX_Start(&req[k]) == MPI_SUCCESS);

    //wait for a request to complete
    int indices[NUMREQ];
    int complete; //number of complete requests
    MPIX_Waitsome(NUMREQ, req, &complete, indices, MPI_STATUS_IGNORE);
    printf("%d %d Wait complete \n", indices[0], complete);
    for(int h=0;h<complete;h++)
    	printf("%d ", indices[h]);
    printf("\n");

   //Testsome -- see if incomplete request is returned (it shouldn't be)
   MPIX_Testsome(NUMREQ, req, &complete, indices,MPI_STATUS_IGNORE);
   printf("%d Test Complete \n", complete);
   for(int i=0; i<complete; i++)
   	printf("%d ",indices[i]);
   printf("\n");

    //start final request
    assert(MPIX_Start(&req[NUMREQ]) == MPI_SUCCESS);

    //wait for first to complete
    assert(MPIX_Waitall(NUMREQ, req, MPI_STATUS_IGNORE) == MPI_SUCCESS);
    printf("Second request complete\n");

    /* compute the sum of the values received */
    for (i = 0, sum = 0.0; i < bufsize; i++)
   	sum += buf[i];

    for(int j=0; j<NUMREQ; j++)
    	assert(MPIX_Request_free(&req[j]) == MPI_SUCCESS);

    //assert(MPIX_Request_free(&req[1]) == MPI_SUCCESS);
    printf("[%d]: #partitions = %d bufsize = %d count = %d sum = %f (%f)\n",
           rank, nparts, bufsize, count, sum, ((double)bufsize*(bufsize+1))/2.0);

  }

  free(buf);

  MPI_Finalize();

  return 0;
}

/* Program that test the performance of partitioned point-to-point
 * operations for different number of partitions.
 * Specify -DMAX_BUFF_SIZE=<bufsize> where bufsize is divisible by
 * all even number between 2 and 28. Eg: 1441440 2882880 5765760 11531520
 * This program does warm up example (the sections with the 'forward'
 * print statements commented out)
 */
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <omp.h>
#include "mpipcl.h"

#define MAX_PARTS 28 /* equal to the no. of cores on the node */
#define RUNS 100 /* no. of times start/wait is called for single init */

int main(){
  //setup
  int *buffer;

  double total_time = 0.0;
  double start, end;
  int rank, parts, buffer_size, provided;
  buffer_size = MAX_BUFF_SIZE;

  MPI_Init_thread(NULL, NULL, MPI_THREAD_SERIALIZED, &provided);
  assert(provided == MPI_THREAD_SERIALIZED);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  buffer = malloc(sizeof(int)*MAX_BUFF_SIZE);
  MPIX_Request* prequest = malloc(sizeof(MPIX_Request));

  for(int i = 0; i<MAX_BUFF_SIZE; i++){
    buffer[i] = i;
  }
  printf("MAX_BUFF_SIZE=%d\n", MAX_BUFF_SIZE);

  // Root acts as sender
  if(rank == 0){
    for(parts = 2; parts <= MAX_PARTS ; parts += 2){
      //Normal Send
      MPI_Barrier(MPI_COMM_WORLD);
      start = MPI_Wtime();
      MPIX_Psend_init(buffer, parts, buffer_size/parts, MPI_INT, 1, 999, MPI_COMM_WORLD, prequest);
      for(int k = 0; k<RUNS; k++){
	MPIX_Start(prequest);

/* #pragma omp parallel for shared(buffer,prequest) num_threads(parts) */
	for (int i = 0; i < parts; i++) {
	  MPIX_Pready(i, prequest);
	}

	MPIX_Wait(prequest, MPI_STATUS_IGNORE);
      }

      MPIX_Request_free(prequest);
      end = MPI_Wtime();

      total_time = end - start;
      //printf("Forward: %d,%d,%d,%f\n",rank, parts, buffer_size/parts, total_time);
    }
  }

  // Other acts as receiver
  if(rank == 1){
    for(parts = 2; parts <= MAX_PARTS ; parts += 2){
      //Normal Send
      MPI_Barrier(MPI_COMM_WORLD);
      start = MPI_Wtime();
      MPIX_Precv_init(buffer, parts, buffer_size/parts, MPI_INT, 0, 999, MPI_COMM_WORLD, prequest);
      for(int k = 0; k<RUNS; k++){
	MPIX_Start(prequest);
	MPIX_Wait(prequest, MPI_STATUS_IGNORE);
      }
      MPIX_Request_free(prequest);
      end = MPI_Wtime();

      total_time = end - start;
      //printf("Forward: %d,%d,%d,%f\n",rank, parts, buffer_size/parts, total_time);
    }
  }

  // Root acts as sender
  if(rank == 0){
    for(parts = MAX_PARTS; parts >= 2 ; parts -= 2){
      //Normal Send
      MPI_Barrier(MPI_COMM_WORLD);
      start = MPI_Wtime();
      MPIX_Psend_init(buffer, parts, buffer_size/parts, MPI_INT, 1, 999, MPI_COMM_WORLD, prequest);
      for(int k = 0; k<RUNS; k++){
	MPIX_Start(prequest);

/* #pragma omp parallel for shared(buffer,prequest) num_threads(parts) */
	for (int i = 0; i < parts; i++) {
	  MPIX_Pready(i, prequest);
	}

	MPIX_Wait(prequest, MPI_STATUS_IGNORE);
      }

      MPIX_Request_free(prequest);
      end = MPI_Wtime();

      total_time = end - start;
      printf("Backward: %d,%d,%d,%f\n",rank, parts, buffer_size/parts, total_time);
    }
  }

  // Other acts as receiver
  if(rank == 1){
    for(parts = MAX_PARTS; parts >= 2 ; parts -= 2){
      //Normal Send
      MPI_Barrier(MPI_COMM_WORLD);
      start = MPI_Wtime();
      MPIX_Precv_init(buffer, parts, buffer_size/parts, MPI_INT, 0, 999, MPI_COMM_WORLD, prequest);
      for(int k = 0; k<RUNS; k++){
	MPIX_Start(prequest);
	MPIX_Wait(prequest, MPI_STATUS_IGNORE);
      }
      MPIX_Request_free(prequest);
      end = MPI_Wtime();

      total_time = end - start;
      printf("Backward: %d,%d,%d,%f\n",rank, parts, buffer_size/parts, total_time);
    }
  }

  free(prequest);
  free(buffer);

  MPI_Finalize();

}

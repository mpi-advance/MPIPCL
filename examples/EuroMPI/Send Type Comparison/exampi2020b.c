#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpipcl.h"

#define MAX_BUFF_SIZE 4194304
#define RUNS 100

int main(){
  //setup
  int* buffer; 

  
  double total_time = 0.0;
  double start, end;  
  int rank, buffer_size;
 
  MPI_Init(NULL,NULL);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    buffer = malloc(MAX_BUFF_SIZE*sizeof(int));
  //Root acts as sender
  if(rank == 0){
    for(buffer_size = 1; buffer_size <= MAX_BUFF_SIZE ; buffer_size<<=1){
     
      //sync send ------------------------------------------------------------------
      MPI_Barrier(MPI_COMM_WORLD);
	  start = MPI_Wtime();
      for(int k = 0; k<RUNS; k++){			
	MPI_Ssend(buffer, buffer_size, MPI_INT, 1, k, MPI_COMM_WORLD);
      }
      end = MPI_Wtime();
	  
      total_time = end - start;
         printf("MPI_Send_init,%d,%d,%d,%f\n",rank,buffer_size,RUNS,total_time);
    }
  }
  
  //Other acts as reciever
  if(rank == 1){
    for(buffer_size = 1; buffer_size <= MAX_BUFF_SIZE ; buffer_size<<=1){
          
		  
      MPI_Barrier(MPI_COMM_WORLD);
	  start = MPI_Wtime();
      for(int k = 0; k<RUNS; k++){			
	   MPI_Recv(buffer, buffer_size, MPI_INT, 0, k, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      end = MPI_Wtime();
	  
      total_time = end - start;
	  //printf("MPI_Ssend,%d,%d,%d,%f,%f,%f\n",rank,buffer_size,RUNS,start,end,total_time);
      
    }
  }
  //----------------------------------------------------------------------------------- 
  
  MPI_Finalize();
  
}

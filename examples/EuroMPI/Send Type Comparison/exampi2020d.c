#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mpipcl.h"

#define MAX_BUFF_SIZE 4194304
#define RUNS 100

int main(){
  //setup
  int* buffer;  
  double total_time = 0.0;
  double start, end;  
  int rank, buffer_size, provided;
 
  MPI_Init_thread(NULL, NULL, MPI_THREAD_SERIALIZED, &provided);
  assert(provided == MPI_THREAD_SERIALIZED);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  MPIX_Request* prequest = malloc(sizeof(MPIX_Request));
  buffer = malloc(MAX_BUFF_SIZE*sizeof(int));
  
  
  
  //Root acts as sender
  if(rank == 0){
   
	for(buffer_size = 1; buffer_size <= MAX_BUFF_SIZE ; buffer_size<<=1){
	  
      //Normal Send
     if(buffer_size > 256){printf("SEND %d Barrier\n", buffer_size);}
      MPI_Barrier(MPI_COMM_WORLD);          
      start = MPI_Wtime();	
      MPIX_Psend_init(buffer, 1, buffer_size, MPI_INT, 1, 999, MPI_COMM_WORLD, prequest);
      for(int k = 0; k<RUNS; k++){
	
		MPIX_Start(prequest);
 	
		MPIX_Pready(0, prequest);
		
                if(buffer_size > 512){printf("SEND %d at Wait\n", buffer_size);}
 		MPIX_Wait(prequest, MPI_STATUS_IGNORE);
      }
      MPIX_Request_free(prequest);
      end = MPI_Wtime();
 
      total_time = end - start;
      printf("MPI_PSend,%d,%d,%d,%f\n",rank,buffer_size,RUNS,total_time);
         
    }
  }
  
  //Other acts as reciever
  if(rank == 1){
    buffer_size = 1;
    for(buffer_size = 1; buffer_size <= MAX_BUFF_SIZE ; buffer_size<<=1){
          
      //Normal Send
      if(buffer_size > 256){printf("RECV %d at Barrier\n", buffer_size);}
      MPI_Barrier(MPI_COMM_WORLD);
      start = MPI_Wtime();
      MPIX_Precv_init(buffer, 1, buffer_size, MPI_INT, 0, 999, MPI_COMM_WORLD, prequest);
      for(int k = 0; k<RUNS; k++){	
			MPIX_Start(prequest);
			MPIX_Wait(prequest, MPI_STATUS_IGNORE); 
      }
      MPIX_Request_free(prequest);
      end = MPI_Wtime();
      
      total_time = end - start;
      //printf("MPI_PSend,%d,%d,%d,%f,%f,%f\n",rank,buffer_size,RUNS,start,end,total_time);
    }
  }
  //----------------------------------------------------------------------------------- 
  
  free(prequest);  
  	  
  MPI_Finalize();
  
}

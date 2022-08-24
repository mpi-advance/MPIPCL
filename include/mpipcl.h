/* Prototype and definitions related to partitioned communication APIs */
#ifndef __MPIPCL__
#define __MPIPCL__
#include <mpi.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

//Useful definitions used multiple locations. 
#define SENDER 0
#define RECEIVER 1

#define ACTIVE 1
#define INACTIVE 0

#define TAGLENGTH 20


//structure to hold message settings if threaded sync is necessary
typedef struct _meta_{
   void* buff;
   int partner;
   int tag;
   MPI_Comm comm;
   MPI_Datatype type;
}meta;


typedef struct _mpix_request {
  int state;
  int side;
  bool* local_status;    //status array - true if external partition is ready 
  int* internal_status; //status array - true if internal partition is ready  
  bool* complete;        //status array - true if internal request has been started. 
  
  int local_parts; //number of partitions visible externally
  int local_size;  //number of items in each partitions
  
  int parts; //number of internal requests to complete
  int size;  //number of items in each internal request
  MPI_Request *request; //array of "size" internal requests to process
  
  //Struct of message data to setup internal requests away from Init function
  struct _meta_* comm_data;
  
  //thread variables to enable background sync if necessary
  pthread_t sync_thread;
  pthread_mutex_t lock;
  int threaded; // status of sync thread "-1"-no_thread, 0-exist, 1-finished

  //function_pointer for proper send functions
  void (*send_fun)(int, struct _mpix_request*);     //trigger when single partition flag trips
  void (*send_catchup_fun)(struct _mpix_request*);  //send all ready partitions without having a particular flag trip.  

}MPIX_Request;



//----------------------------------------------------------------------------------------------


int MPIX_Psend_init(void *buf, int partitions, MPI_Count count,
		    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIX_Request *request);

int MPIX_Precv_init(void *buf, int partitions, MPI_Count count,
		    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, 
		    MPIX_Request *request);

int MPIX_Pready(int partition, MPIX_Request *request);

int MPIX_Pready_range(int partition_low, int partition_high, MPIX_Request *request);

int MPIX_Pready_list(int length, int array_of_partitions[], MPIX_Request *request);

int MPIX_Parrived(MPIX_Request *request, int partition, int *flag);

 
int MPIX_Start(MPIX_Request *request);
int MPIX_Startall(int count, MPIX_Request array_of_requests[]);

int MPIX_Wait(MPIX_Request *request, MPI_Status *status);
int MPIX_Waitall(int count, MPIX_Request array_of_requests[],
		MPI_Status array_of_statuses[]);
int MPIX_Waitany(int count, MPIX_Request array_of_requests[],
		int *index, MPI_Status *status);
int MPIX_Waitsome(int incount, MPIX_Request array_of_requests[],
		 int *outcount, int array_of_indices[],
		 MPI_Status array_of_statuses[]);

int MPIX_Test(MPIX_Request *request, int *flag, MPI_Status *status);
int MPIX_Testall(int count, MPIX_Request array_of_requests[],
		int *flag, MPI_Status array_of_statuses[]);
int MPIX_Testany(int count, MPIX_Request array_of_requests[],
		int *index, int *flag, MPI_Status *status);
int MPIX_Testsome(int incount, MPIX_Request array_of_requests[],
		 int *outcount, int array_of_indices[],
		 MPI_Status array_of_statuses[]);

int MPIX_Request_free(MPIX_Request *request);

//functions current defined outside of mpipcl
//setup.c
void prep(void *buf, int partitions, MPI_Count count, MPI_Datatype datatype, int opp, int tag, MPI_Info info, MPI_Comm comm, MPIX_Request *request);
int sync_driver(MPI_Info info, MPIX_Request* request);
void internal_setup(MPIX_Request* request);
void reset_status(MPIX_Request* request);

//sync.c
void sync_hard(int option, MPIX_Request* request);
void sync_side(int driver, MPIX_Request* request);
void* threaded_sync_driver(void* args);

//send.c
  //send functions
  void send_ready(MPIX_Request* request);
  void simple_send(int id, MPIX_Request* request);
  void general_send(int id, MPIX_Request* request);

  //remap functions
  void map_send_buffer_percent(int id, MPIX_Request* request);
  void map_send_buffer_bool(int id, MPIX_Request* request);
  void map_send_buffer_count(int id, MPIX_Request* request);
  int map_recv_buffer(int id, MPIX_Request* request);

#endif

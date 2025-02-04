/* Prototype and definitions related to partitioned communication APIs */
#ifndef __MPIPCL__
#define __MPIPCL__

#ifdef __cplusplus
extern "C"
{
#endif

#include <mpi.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define MPIPCL_TAG_LENGTH 20

#ifndef MPIPCL_UNIQUE_NAMES
#define MPIPCL_REQUEST MPIX_Request
#define MPIPCL_NAMESPACE MPIX
#else
#define MPIPCL_REQUEST MPIP_Request
#define MPIPCL_NAMESPACE MPIP
#endif

// Determines name of function based on "MPIPCL_UNIQUE_NAMES"
#define MPIPCL(name) MPIPCL_NAMESPACE##name

// structure to hold message settings if threaded sync is necessary
typedef struct _meta_
{
  void *buff;
  int partner;
  int tag;
  MPI_Comm comm;
  MPI_Datatype type;
} meta;

// enums for mpix request attributes
enum P2P_Side
{
  SENDER = 0,
  RECEIVER = 1
};
enum Activation
{
  INACTIVE = 0,
  ACTIVE = 1
};
enum Thread_Status
{
  NONE = -1,
  RUNNING = 0,
  FINISHED = 1
};

typedef struct _mpipcl_request
{
  enum Activation state;
  enum P2P_Side side;
  bool *local_status;   // status array - true if external partition is ready
  int *internal_status; // status array - true if internal partition is ready
  bool *complete;       // status array - true if internal request has been started.

  int local_parts; // number of partitions visible externally
  int local_size;  // number of items in each partitions

  int parts;            // number of internal requests to complete
  int size;             // number of items in each internal request
  MPI_Request *request; // array of "size" internal requests to process

  // Struct of message data to setup internal requests away from Init function
  struct _meta_ *comm_data;

  // thread variables to enable background sync if necessary
  pthread_t sync_thread;
  pthread_mutex_t lock;
  enum Thread_Status threaded; // status of sync thread "-1"-no_thread, 0-exist, 1-finished
} MPIPCL_REQUEST;

//----------------------------------------------------------------------------------------------

int MPIPCL(_Psend_init)(void *buf, int partitions, MPI_Count count,
                        MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST *request);

int MPIPCL(_Precv_init)(void *buf, int partitions, MPI_Count count,
                        MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info,
                        MPIPCL_REQUEST *request);

int MPIPCL(_Pready)(int partition, MPIPCL_REQUEST *request);

int MPIPCL(_Pready_range)(int partition_low, int partition_high, MPIPCL_REQUEST *request);

int MPIPCL(_Pready_list)(int length, int array_of_partitions[], MPIPCL_REQUEST *request);

int MPIPCL(_Parrived)(MPIPCL_REQUEST *request, int partition, int *flag);

int MPIPCL(_Start)(MPIPCL_REQUEST *request);
int MPIPCL(_Startall)(int count, MPIPCL_REQUEST array_of_requests[]);

int MPIPCL(_Wait)(MPIPCL_REQUEST *request, MPI_Status *status);
int MPIPCL(_Waitall)(int count, MPIPCL_REQUEST array_of_requests[],
                      MPI_Status array_of_statuses[]);
int MPIPCL(_Waitany)(int count, MPIPCL_REQUEST array_of_requests[],
                      int *index, MPI_Status *status);
int MPIPCL(_Waitsome)(int incount, MPIPCL_REQUEST array_of_requests[],
                      int *outcount, int array_of_indices[],
                      MPI_Status array_of_statuses[]);

int MPIPCL(_Test)(MPIPCL_REQUEST *request, int *flag, MPI_Status *status);
int MPIPCL(_Testall)(int count, MPIPCL_REQUEST array_of_requests[],
                      int *flag, MPI_Status array_of_statuses[]);
int MPIPCL(_Testany)(int count, MPIPCL_REQUEST array_of_requests[],
                      int *index, int *flag, MPI_Status *status);
int MPIPCL(_Testsome)(int incount, MPIPCL_REQUEST array_of_requests[],
                      int *outcount, int array_of_indices[],
                      MPI_Status array_of_statuses[]);

int MPIPCL(_Request_free)(MPIPCL_REQUEST *request);

// functions current defined outside of mpipcl
// setup.c
void prep(void *buf, int partitions, MPI_Count count, MPI_Datatype datatype, int opp, int tag, MPI_Comm comm, MPIPCL_REQUEST *request);
int sync_driver(MPI_Info info, MPIPCL_REQUEST *request);
void internal_setup(MPIPCL_REQUEST *request);
void reset_status(MPIPCL_REQUEST *request);

// sync.c
void sync_hard(int option, MPIPCL_REQUEST *request);
void sync_side(enum P2P_Side driver, MPIPCL_REQUEST *request);
void *threaded_sync_driver(void *args);

// send.c
// send functions
void send_ready(MPIPCL_REQUEST *request);
void general_send(int id, MPIPCL_REQUEST *request);

// remap functions
int map_recv_buffer(int id, MPIPCL_REQUEST *request);

// debug functions
#if defined(WITH_DEBUG)
#define MPIPCL_DEBUG(X, ...) printf(X, ##__VA_ARGS__);
#else
#define MPIPCL_DEBUG(X, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif

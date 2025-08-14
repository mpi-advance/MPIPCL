/* Prototype and definitions related to partitioned communication APIs */
#ifndef __MPIPCL__
#define __MPIPCL__

#ifdef __cplusplus
extern "C" {
#endif

#include <mpi.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
#include <stdatomic.h>
#endif
#include <assert.h>

#define MPIPCL_TAG_LENGTH 20

// structure to hold message settings if threaded sync is necessary
typedef struct _meta_
{
    void* buff;
    int partner;
    int tag;
    MPI_Comm comm;
    MPI_Datatype type;
} meta;

// enums for MPIP request attributes
enum P2P_Side
{
    SENDER   = 0,
    RECEIVER = 1
};
enum Activation
{
    INACTIVE = 0,
    ACTIVE   = 1
};
enum Thread_Status
{
    NONE     = -1,
    RUNNING  = 0,
    FINISHED = 1
};

typedef struct _MPIP_Request
{
    enum Activation state;
    enum P2P_Side side;
    bool* local_status;  // status array - true if external partition is ready
#ifdef __cplusplus
    void* internal_status;  // C++ can't use "atomic_int" from C, so let's just
                            // make it void *
#else
    atomic_int* internal_status;  // status array - true if internal partition is ready
#endif
    bool* complete;  // status array - true if internal request has been started.

    int local_parts;  // number of partitions visible externally
    int local_size;   // number of items in each partitions

    int parts;             // number of internal requests to complete
    int size;              // number of items in each internal request
    MPI_Request* request;  // array of "size" internal requests to process

    // Struct of message data to setup internal requests away from Init function
    struct _meta_* comm_data;

    // thread variables to enable background sync if necessary
    pthread_t sync_thread;
    pthread_mutex_t lock;
    // status of sync thread "-1"-no_thread, 0-exist, 1-finished
    enum Thread_Status threaded;
} MPIP_Request;

//----------------------------------------------------------------------------------------------

int MPIP_Psend_init(void* buf,
                    int partitions,
                    MPI_Count count,
                    MPI_Datatype datatype,
                    int dest,
                    int tag,
                    MPI_Comm comm,
                    MPI_Info info,
                    MPIP_Request* request);

int MPIP_Precv_init(void* buf,
                    int partitions,
                    MPI_Count count,
                    MPI_Datatype datatype,
                    int dest,
                    int tag,
                    MPI_Comm comm,
                    MPI_Info info,
                    MPIP_Request* request);

int MPIP_Pready(int partition, MPIP_Request* request);

int MPIP_Pready_range(int partition_low, int partition_high, MPIP_Request* request);

int MPIP_Pready_list(int length, int array_of_partitions[], MPIP_Request* request);

int MPIP_Parrived(MPIP_Request* request, int partition, int* flag);

int MPIP_Start(MPIP_Request* request);
int MPIP_Startall(int count, MPIP_Request array_of_requests[]);

int MPIP_Wait(MPIP_Request* request, MPI_Status* status);
int MPIP_Waitall(int count,
                 MPIP_Request array_of_requests[],
                 MPI_Status array_of_statuses[]);
int MPIP_Waitany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 MPI_Status* status);
int MPIP_Waitsome(int incount,
                  MPIP_Request array_of_requests[],
                  int* outcount,
                  int array_of_indices[],
                  MPI_Status array_of_statuses[]);

int MPIP_Test(MPIP_Request* request, int* flag, MPI_Status* status);
int MPIP_Testall(int count,
                 MPIP_Request array_of_requests[],
                 int* flag,
                 MPI_Status array_of_statuses[]);
int MPIP_Testany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 int* flag,
                 MPI_Status* status);
int MPIP_Testsome(int incount,
                  MPIP_Request array_of_requests[],
                  int* outcount,
                  int array_of_indices[],
                  MPI_Status array_of_statuses[]);

int MPIP_Request_free(MPIP_Request* request);

// functions current defined outside of mpipcl
// setup.c
void prep(void* buf,
          int partitions,
          MPI_Count count,
          MPI_Datatype datatype,
          int opp,
          int tag,
          MPI_Comm comm,
          MPIP_Request* request);
int sync_driver(MPI_Info info, MPIP_Request* request);
void internal_setup(MPIP_Request* request);
void reset_status(MPIP_Request* request);

// sync.c
void sync_hard(int option, MPIP_Request* request);
void sync_side(enum P2P_Side driver, MPIP_Request* request);
void* threaded_sync_driver(void* args);

// send.c
// send functions
void send_ready(MPIP_Request* request);
void general_send(int id, MPIP_Request* request);

// remap functions
int map_recv_buffer(int id, MPIP_Request* request);

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

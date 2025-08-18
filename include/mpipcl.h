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
/**
* Structure to hold message settings if additional sycnchronization is necessary. 
* Contains necessary information to set up a 2-sided communication channel. 
*/
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


/**
* the MPIP_Request object used in a majority of the MPIPCL API calls
* Contains a an array of internal MPI_Requests for tracking progress of partition transmission
* Note: local_parts * local_size should equal parts*size
*/

typedef struct _MPIP_Request
{
    /**
	* Activation state of the request, 0= INACTIVE, 1=ACTIVE
	*/
	enum Activation state;
	
	/**
	* Side of the communication the request is providing. 0=SENDER, 1=RECEIVER
	*/
    enum P2P_Side side;
	
    /**
	* Array of booleans representing the ready status of each external partition. 
	* If the data is marked as ready be sent, then the status is 1 else 0. 
	*/
	bool* local_status;  // status array - true if external partition is ready

	/**
	* Array of booleans representing the ready status of each internal partition. 
	* If the status is 1, then the partition is ready to be sent. A partition should only be marked as 1
	* if all external partitions it depends on are marked as ready. 
	* Type depends on the language used during compiliation
	* atomic_int* is used for C compilers. 
	* Void* is used for C++ compilers as atomic_int is not defined in C++. 
	*/
#ifdef __cplusplus
    void* internal_status;  // C++ can't use "atomic_int" from C, so let's just
                            // make it void *
#else
    atomic_int* internal_status;  // status array - true if internal partition is ready
#endif

	/**
	* Array of booleans representing the completion status of each  partition. 
	* If the partition has been sent, then the status 1, else 0. 
	* Should be updated based on status of the related request inside MPIPCL_Request
	*/
    bool* complete;  // status array - true if internal request has been started.

	/**
	* Number of externally viewed partitions. 
	*/
    int local_parts;  // number of partitions visible externally
	
	/**
	* Number of elements in each externally viewed partition. 
	*/
    int local_size;   // number of items in each partitions
	
	/**
	* Number of internal partitions, each internal partition is sent as seperate internal send.  
	*/
    int parts;             // number of internal requests to complete
	
	/**
	* The number of elements in each internal partition.  
	*/
    int size;              // number of items in each internal request
	
	/**
	* Array of internal requests, one request per internal partition. 
	* All internal requests must be complete for the MPIPCL_Request to be considered complete. 
	*/
    MPI_Request* request;  // array of "size" internal requests to process

	/**
    * Struct of message data to setup internal requests away from Init function
    */
	struct _meta_* comm_data;

	/**
	* Handle to thread enabling background progress and non-blocking behavior of the init functions. 
	* No data will be transfered until the thread completes its setup and finishes. 
	*/
    pthread_t sync_thread;
	/**
	* Mutex to handle access to background progress thread
    */
	pthread_mutex_t lock;
	/**
	* variable to check status of progress thread. -1 = no_thread, 0 = thread_exists, 1 = finished.
	*/	
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

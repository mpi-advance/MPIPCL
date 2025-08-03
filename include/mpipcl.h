/* Prototype and definitions related to partitioned communication APIs */
#ifndef __MPIPCL__
#define __MPIPCL__

#ifdef __cplusplus
extern "C" {
#endif

#include "MPIAdvance/base.h"

#include <mpi.h>

#define MPIPCL_TAG_LENGTH 20

//----------------------------------------------------------------------------------------------

int MPIA_Psend_init(void* buf, int partitions, MPI_Count count,
                        MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                        MPI_Info info, MPIA_Request* request);

int MPIA_Precv_init(void* buf, int partitions, MPI_Count count,
                        MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                        MPI_Info info, MPIA_Request* request);

int MPIA_Pready(int partition, MPIA_Request* request);

int MPIA_Pready_range(int partition_low, int partition_high,
                          MPIA_Request* request);

int MPIA_Pready_list(int length, int array_of_partitions[],
                         MPIA_Request* request);

int MPIA_Parrived(MPIA_Request* request, int partition, int* flag);

//int MPIPCL(_Waitany)(int count, MPIA_Request array_of_requests[], int* index,
//                     MPI_Status* status);
//int MPIPCL(_Waitsome)(int incount, MPIA_Request array_of_requests[],
//                      int* outcount, int array_of_indices[],
//                      MPI_Status array_of_statuses[]);

//int MPIPCL(_Testall)(int count, MPIA_Request array_of_requests[], int* flag,
//                     MPI_Status array_of_statuses[]);
//int MPIPCL(_Testany)(int count, MPIA_Request array_of_requests[], int* index,
//                     int* flag, MPI_Status* status);
//int MPIPCL(_Testsome)(int incount, MPIA_Request array_of_requests[],
//                      int* outcount, int array_of_indices[],
//                      MPI_Status array_of_statuses[]);

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

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

#ifdef __cplusplus
}
#endif

#endif

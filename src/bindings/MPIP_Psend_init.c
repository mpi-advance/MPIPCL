/**
* @file mpipcl.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Init functions - call function in setup.c
int MPIP_Psend_init(void* buf,
                    int partitions,
                    MPI_Count count,
                    MPI_Datatype datatype,
                    int dest,
                    int tag,
                    MPI_Comm comm,
                    MPI_Info info,
                    MPIP_Request* request)
{
    request->side = SENDER;
    prep(buf, partitions, count, datatype, dest, tag, comm, request);
    sync_driver(info, request);

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
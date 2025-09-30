/**
* @file MPIP_Precv_init.c
*/

#include "mpipcl.h"
#include "pt2pt.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Precv_init(void* buf,
                    int partitions,
                    MPI_Count count,
                    MPI_Datatype datatype,
                    int dest,
                    int tag,
                    MPI_Comm comm,
                    MPI_Info info,
                    MPIP_Request* request)
{
    request->side = RECEIVER;
    prep(buf, partitions, count, datatype, dest, tag, comm, request);
    sync_driver(info, request);

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
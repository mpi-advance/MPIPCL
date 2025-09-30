/**
* @file MPIP_Pready_range.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Pready_range(int partition_low, int partition_high, MPIP_Request* request)
{
    for (int i = partition_low; i <= partition_high; i++)
    {
        int ret_val = MPIP_Pready(i, request);
        assert(MPI_SUCCESS == ret_val);
    }
    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
/**
* @file MPIP_Waitall.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Waitall(int count,
                 MPIP_Request array_of_requests[],
                 MPI_Status array_of_statuses[])
{
    MPIPCL_DEBUG("Will wait on: %d\n", count);
    for (int i = 0; i < count; i++)
    { /* NOTE: array of MPI_Status objects is not updated */
        int ret_val = MPIP_Wait(&array_of_requests[i], MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == ret_val);
    }

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
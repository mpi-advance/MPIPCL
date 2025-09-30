/**
* @file MPIP_Startall.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Startall(int count, MPIP_Request array_of_requests[])
{
    for (int i = 0; i < count; i++)
    {
        int ret_val = MPIP_Start(&array_of_requests[i]);
        assert(MPI_SUCCESS == ret_val);
    }

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
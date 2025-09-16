/**
* @file MPIP_Waitany.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Waitany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 MPI_Status* status)
{
    MPIPCL_DEBUG("MPIP_Waitany\n");
    int flag = 0;
    while (!flag)
    {
        for (int i = 0; i < count; i++)
        { /* NOTE: MPI_Status object is not updated */
            int ret_val = MPIP_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE);
            assert(MPI_SUCCESS == ret_val);
            if (flag == 1)
            {
                *index = i;
                break;
            }
        }
    }

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
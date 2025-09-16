/**
* @file mpipcl.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Testany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 int* flag,
                 MPI_Status* status)
{
    MPIPCL_DEBUG("MPIP_Testany\n");
    // for each MPIP_Request in provided array
    for (int i = 0; i < count; i++)
    {
        int ret_val = MPIP_Test(&array_of_requests[i], flag, MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == ret_val);
        if (*flag == 1)
        {
            *index = i;
            break;
        }
    }
    return MPI_SUCCESS;
}


#ifdef __cplusplus
}
#endif
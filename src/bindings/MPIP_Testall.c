/**
* @file mpipcl.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif


int MPIP_Testall(int count,
                 MPIP_Request array_of_requests[],
                 int* flag,
                 MPI_Status array_of_statuses[])
{
    MPIPCL_DEBUG("MPIP_Testall\n");
    int myflag;
    *flag = 1;
    for (int i = 0; i < count; i++)
    {
        int ret_val = MPIP_Test(&array_of_requests[i], &myflag, MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == ret_val);
        *flag = *flag & myflag;
    }

    /* NOTE: array of MPI_Status objects is not updated */
    return MPI_SUCCESS;
}



#ifdef __cplusplus
}
#endif
/**
* @file MPIP_Waitsome.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif
int MPIP_Waitsome(int incount,
                  MPIP_Request array_of_requests[],
                  int* outcount,
                  int array_of_indices[],
                  MPI_Status array_of_statuses[])
{
    MPIPCL_DEBUG("MPIP_Waitsome\n");
    int j = 0, flag = 0;

    *outcount = 0;
    while (*outcount < 1)
    {
        for (int i = 0; i < incount; i++)
        {
            int ret_val = MPIP_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE);
            assert(MPI_SUCCESS == ret_val);
            if (flag == 1)
            {
                *outcount           = *outcount + 1;
                array_of_indices[j] = i;
                j++;
            }
        }
    }
    return MPI_SUCCESS;
}


#ifdef __cplusplus
}
#endif
/**
* @file mpipcl.c
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

int MPIP_Pready_list(int length, int array_of_partitions[], MPIP_Request* request)
{
    for (int i = 0; i < length; i++)
    {
        int ret_val = MPIP_Pready(array_of_partitions[i], request);
        assert(MPI_SUCCESS == ret_val);
    }
    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
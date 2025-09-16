/**
* @file mpipcl.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Other test function calls basic test, modifying MPIP_Test will propagate the
// changes.
int MPIP_Test(MPIP_Request* request, int* flag, MPI_Status* status)
{
    *flag = 0;

    if (request == NULL || request->state == INACTIVE)
    {
        MPIPCL_DEBUG("Early MPIP Test exit due to null or inactive request: %p\n",
                     (void*)request);
        return MPI_SUCCESS;
    }

    pthread_mutex_lock(&request->lock);
    int t_status = request->threaded;
    pthread_mutex_unlock(&request->lock);

    // if not synced, return false
    if (t_status == 0)
    {
        MPIPCL_DEBUG("Early MPIP Test exit due to not synched\n");
        return MPI_SUCCESS;
    }

    // else test status of each request in communication.]
    int ret_val =
        MPI_Testall(request->parts, request->request, flag, MPI_STATUSES_IGNORE);
    assert(MPI_SUCCESS == ret_val);

    if (*flag == 1)
    {
        request->state = INACTIVE;
    }

    /* NOTE: MPI_Status object is not updated */
    return MPI_SUCCESS;
}


#ifdef __cplusplus
}
#endif
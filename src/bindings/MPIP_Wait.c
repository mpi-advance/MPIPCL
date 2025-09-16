/**
* @file mpipcl.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Other Wait function calls basic test, modifying MPIP_Wait will propagate the
// changes.
int MPIP_Wait(MPIP_Request* request, MPI_Status* status)
{
    // if(request == NULL || request -> state == INACTIVE) {return MPI_SUCCESS;}
    MPIPCL_DEBUG("Inside MPIP Wait: %d \n", request->side);
    pthread_mutex_lock(&request->lock);
    enum Thread_Status t_status = request->threaded;
    pthread_mutex_unlock(&request->lock);

    // if thread has not completed.
    if (t_status == RUNNING)
    {
        // wait until thread completes and joins
        pthread_join(request->sync_thread, NULL);
    }

    // once setup is complete, wait on all internal partitions.
    MPIPCL_DEBUG(
        "Waiting on %d reqs at address %p\n", request->parts, (void*)request->request);
    int ret_val = MPI_Waitall(request->parts, request->request, MPI_STATUSES_IGNORE);
    assert(MPI_SUCCESS == ret_val);

    // set state to inactive.
    request->state = INACTIVE;

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
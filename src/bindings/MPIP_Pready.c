/**
* @file MPIP_Pready.c
*/

#include "mpipcl.h"
#include "pt2pt.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Pready(int partition, MPIP_Request* request)
{
    MPIPCL_DEBUG("INSIDE PREADY\n");
    // check for calling conditions
    assert(request->side == SENDER && request->state == ACTIVE);

    // set local status
    request->local_status[partition] = 1;

    // local request and check status
    pthread_mutex_lock(&request->lock);
    enum Thread_Status thread_status = request->threaded;
    pthread_mutex_unlock(&request->lock);

    // if sync complete - call send function
    if (thread_status != RUNNING)
    {
        general_send(partition, request);
    }
    else
    {
        MPIPCL_DEBUG("%d delayed\n", partition);
    }

    return MPI_SUCCESS;
}


#ifdef __cplusplus
}
#endif
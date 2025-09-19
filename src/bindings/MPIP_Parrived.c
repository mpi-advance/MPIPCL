/**
* @file MPIP_Parrived
*/

#include "mpipcl.h"
#include "pt2pt.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Parrived(MPIP_Request* request, int partition, int* flag)
{
    assert(request->side != SENDER);

    pthread_mutex_lock(&request->lock);
    enum Thread_Status status = request->threaded;
    pthread_mutex_unlock(&request->lock);

    // if not synced - return early;
    if (status == RUNNING)
    {
        *flag = 0;
        return MPI_SUCCESS;
    }

    // else use mapping function to check status
    *flag = map_recv_buffer(partition, request);

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
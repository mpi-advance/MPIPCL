/**
* @file mpipcl.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

int MPIP_Start(MPIP_Request* request)
{
    MPIPCL_DEBUG("MPIP START CALLED: %d\n", request->side);
    pthread_mutex_lock(&request->lock);
    enum Thread_Status thread_status = request->threaded;
    request->state                   = ACTIVE;
    pthread_mutex_unlock(&request->lock);

    // setup complete
    if (thread_status != RUNNING)
    {
        // reset ready flags
        for (int i = 0; i < request->parts; i++)
        {
            request->internal_status[i] = 0;
        }
        reset_status(request);
        // if receiver start recv requests.
        if (request->side == RECEIVER)
        {
            MPIPCL_DEBUG("USER THREAD IS STARTING RECV:%d\n", request->parts);
            int ret_val = MPI_Startall(request->parts, request->request);
            assert(MPI_SUCCESS == ret_val);
        }
    }

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
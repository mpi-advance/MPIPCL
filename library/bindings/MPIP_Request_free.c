/**
* @file MPIP_Request_free.c
*/

#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

// cleanup function
int MPIP_Request_free(MPIP_Request* request)
{
    // clear internal array of requests
    for (int i = 0; i < request->parts; i++)
    {
        int ret_val = MPI_Request_free(&request->request[i]);
        assert(MPI_SUCCESS == ret_val);
    }

    // free request buffer
    free(request->request);

    // free internal request status buffers
    free(request->local_status);
    free(request->internal_status);

    // free comm_data -- //could this be freed earlier?
    free(request->comm_data);

    // clean up hanging threads and controls
    pthread_mutex_destroy(&request->lock);

    return MPI_SUCCESS;
}


#ifdef __cplusplus
}
#endif
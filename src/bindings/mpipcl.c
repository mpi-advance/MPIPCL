#include "mpipcl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Initial implementation of partitioned communication API */

// Init functions - call function in setup.c
int MPIP_Psend_init(void* buf, int partitions, MPI_Count count,
                        MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                        MPI_Info info, MPIP_Request* request)
{
    request->side = SENDER;
    prep(buf, partitions, count, datatype, dest, tag, comm, request);
    sync_driver(info, request);

    return MPI_SUCCESS;
}

int MPIP_Precv_init(void* buf, int partitions, MPI_Count count,
                        MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                        MPI_Info info, MPIP_Request* request)
{
    request->side = RECEIVER;
    prep(buf, partitions, count, datatype, dest, tag, comm, request);
    sync_driver(info, request);

    return MPI_SUCCESS;
}

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

int MPIP_Pready_range(int partition_low, int partition_high,
                          MPIP_Request* request)
{
    for (int i = partition_low; i <= partition_high; i++)
    {
        int ret_val = MPIP_Pready(i, request);
        assert(MPI_SUCCESS == ret_val);
    }
    return MPI_SUCCESS;
}

int MPIP_Pready_list(int length, int array_of_partitions[],
                         MPIP_Request* request)
{
    for (int i = 0; i < length; i++)
    {
        int ret_val = MPIP_Pready(array_of_partitions[i], request);
        assert(MPI_SUCCESS == ret_val);
    }
    return MPI_SUCCESS;
}

// calls functions from send.c
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

int MPIP_Startall(int count, MPIP_Request array_of_requests[])
{
    for (int i = 0; i < count; i++)
    {
        int ret_val = MPIP_Start(&array_of_requests[i]);
        assert(MPI_SUCCESS == ret_val);
    }

    return MPI_SUCCESS;
}

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
    MPIPCL_DEBUG("Waiting on %d reqs at address %p\n", request->parts,
                 (void*)request->request);
    int ret_val =
        MPI_Waitall(request->parts, request->request, MPI_STATUSES_IGNORE);
    assert(MPI_SUCCESS == ret_val);

    // set state to inactive.
    request->state = INACTIVE;

    return MPI_SUCCESS;
}

int MPIP_Waitall(int count, MPIP_Request array_of_requests[],
                     MPI_Status array_of_statuses[])
{
    MPIPCL_DEBUG("Will wait on: %d\n", count);
    for (int i = 0; i < count; i++)
    { /* NOTE: array of MPI_Status objects is not updated */
        int ret_val = MPIP_Wait(&array_of_requests[i], MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == ret_val);
    }

    return MPI_SUCCESS;
}

int MPIP_Waitany(int count, MPIP_Request array_of_requests[], int* index,
                     MPI_Status* status)
{
    MPIPCL_DEBUG("MPIP_Waitany\n");
    int flag = 0;
    while (!flag)
    {
        for (int i = 0; i < count; i++)
        { /* NOTE: MPI_Status object is not updated */
            int ret_val =
                MPIP_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE);
            assert(MPI_SUCCESS == ret_val);
            if (flag == 1)
            {
                *index = i;
                break;
            }
        }
    }

    return MPI_SUCCESS;
}

int MPIP_Waitsome(int incount, MPIP_Request array_of_requests[],
                      int* outcount, int array_of_indices[],
                      MPI_Status array_of_statuses[])
{
    MPIPCL_DEBUG("MPIP_Waitsome\n");
    int j = 0, flag = 0;

    *outcount = 0;
    while (*outcount < 1)
    {
        for (int i = 0; i < incount; i++)
        {
            int ret_val =
                MPIP_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE);
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

// Other test function calls basic test, modifying MPIP_Test will propagate the
// changes.
int MPIP_Test(MPIP_Request* request, int* flag, MPI_Status* status)
{
    *flag = 0;

    if (request == NULL || request->state == INACTIVE)
    {
        MPIPCL_DEBUG(
            "Early MPIP Test exit due to null or inactive request: %p\n",
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
    int ret_val = MPI_Testall(request->parts, request->request, flag,
                              MPI_STATUSES_IGNORE);
    assert(MPI_SUCCESS == ret_val);

    if (*flag == 1)
        request->state = INACTIVE;

    /* NOTE: MPI_Status object is not updated */
    return MPI_SUCCESS;
}

int MPIP_Testall(int count, MPIP_Request array_of_requests[], int* flag,
                     MPI_Status array_of_statuses[])
{
    MPIPCL_DEBUG("MPIP_Testall\n");
    int myflag;
    *flag = 1;
    for (int i = 0; i < count; i++)
    {
        int ret_val =
            MPIP_Test(&array_of_requests[i], &myflag, MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == ret_val);
        *flag = *flag & myflag;
    }

    /* NOTE: array of MPI_Status objects is not updated */
    return MPI_SUCCESS;
}

int MPIP_Testany(int count, MPIP_Request array_of_requests[], int* index,
                     int* flag, MPI_Status* status)
{
    MPIPCL_DEBUG("MPIP_Testany\n");
    // for each MPIP_Request in provided array
    for (int i = 0; i < count; i++)
    {
        int ret_val =
            MPIP_Test(&array_of_requests[i], flag, MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == ret_val);
        if (*flag == 1)
        {
            *index = i;
            break;
        }
    }
    return MPI_SUCCESS;
}

int MPIP_Testsome(int incount, MPIP_Request array_of_requests[],
                      int* outcount, int array_of_indices[],
                      MPI_Status array_of_statuses[])
{
    MPIPCL_DEBUG("MPIP_Testsome\n");
    int j = 0, flag = 0;
    *outcount = 0;
    for (int i = 0; i < incount; i++)
    {
        int ret_val =
            MPIP_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE);
        assert(MPI_SUCCESS == ret_val);
        if (flag == 1)
        {
            array_of_indices[*outcount] = i;
            *outcount                   = *outcount + 1;
            j++;
        }
    }
    return MPI_SUCCESS;
}

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
    free(request->complete);

    // free comm_data -- //could this be freed earlier?
    free(request->comm_data);

    // clean up hanging threads and controls
    pthread_mutex_destroy(&request->lock);

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
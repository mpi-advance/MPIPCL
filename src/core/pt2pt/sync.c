/**
 * @file sync.c
 */

#include "mpipcl.h"
#include "pt2pt.h"
// implemented synchronization modes

void sync_hard(int option, MPIP_Request* request)
{
    request->parts = option;
    request->size  = request->local_parts * request->local_size / option;
}

// Send/Receive sync data to/from partner thread
void sync_side(enum P2P_Side driver, MPIP_Request* request)
{
    // extract comm data from meta block
    int partner   = request->comm_data->partner;
    MPI_Comm comm = request->comm_data->comm;
    // Package and send data if driver.
    int syncdata[2];
    if (request->side == driver)
    {
        syncdata[0] = request->local_parts;
        syncdata[1] = request->local_size;
        MPI_Send(syncdata, 2, MPI_INT, partner, 990, comm);
    }
    else
    {
        MPI_Recv(syncdata, 2, MPI_INT, partner, 990, comm, MPI_STATUS_IGNORE);
    }

    // copy data into proper location on request
    request->parts = syncdata[0];
    request->size  = syncdata[1];

    MPIPCL_DEBUG(
        "%d : sync data received %d %d \n", request->side, request->parts, request->size);
}

// instructions for synchronization thread
// defaults to sync_side.
void* threaded_sync_driver(void* args)
{
    // Convert blob to correct type
    MPIP_Request* request = (MPIP_Request*)args;

    sync_side(!request->side, request);
    internal_setup(request);

    // lock and check request status.
    pthread_mutex_lock(&request->lock);
    // if request is active(MPI_Start), run catchup tasks.
    if (request->side == RECEIVER && request->state == ACTIVE)
    {
        MPIPCL_DEBUG("%d THREAD IS STARTING RECVS:%d \n", request->side, request->size);
        int ret_val = MPI_Startall(request->parts, request->request);
        assert(MPI_SUCCESS == ret_val);
    }
    else if (request->side == SENDER && request->state == ACTIVE)
    {
        MPIPCL_DEBUG("%d THREAD IS STARTING SENDS:%d \n", request->side, request->size)
        send_ready(request);
    }
    // once caught up, signal thread completion and return.
    request->threaded = FINISHED;
    pthread_mutex_unlock(&request->lock);

    return NULL;
}

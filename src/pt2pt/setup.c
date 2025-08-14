#include "mpipcl.h"

// functions used by both side of the communication.
// called by Init function in mpipcl.c
// calls functions in sync.c

// fill in default values and bundle message data
void prep(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int opp,
          int tag, MPI_Comm comm, MPIP_Request* request)
{
    /* update partitioned request object with default values*/
    request->state       = INACTIVE;
    request->local_size  = count;
    request->local_parts = partitions;
    request->parts       = 1;  // default to 1
    request->threaded    = NONE;

    /*create & set local status buffer*/
    request->local_status =
        (bool*)malloc(sizeof(bool) * partitions);  // >100 if external partition is ready

    /*set overall request lock*/
    int ret_val = pthread_mutex_init(&request->lock, NULL);
    assert(0 == ret_val);

    // Record mesage options for future request creation
    meta* mesage    = (meta*)malloc(sizeof(meta));
    mesage->buff    = buf;
    mesage->partner = opp;
    mesage->tag     = tag;
    mesage->comm    = comm;
    mesage->type    = datatype;

    // attach mesage data to request object
    request->comm_data = mesage;

    // mutex needed for anything more than trivial mapping case.
    pthread_mutex_init(&request->lock, NULL);  // create mutex

    MPIPCL_DEBUG("%d : PREP COMPLETE\n", request->side);
}

// decode info object and call appropriate sync operation
int sync_driver(MPI_Info info, MPIP_Request* request)
{
    int flag;
    char mode[10];
    char option[10];

    // check for info object
    if (info == MPI_INFO_NULL)
    {
        MPIPCL_DEBUG("NULL INFO detected\n");
        sync_hard(1, request);
    }
    else
    {
        // if exist get selected mode
        MPI_Info_get(info, "PMODE", MPIPCL_TAG_LENGTH, mode, &flag);

        // Change behavior based on key - update convert to ENUM?
        if (strcmp("HARD", mode) == 0)
        {
            MPIPCL_DEBUG("HARD INFO detected\n");
            MPI_Info_get(info, "SET", MPIPCL_TAG_LENGTH, option, &flag);
            assert(flag == 1);

            int set = atoi(option);
            if (set <= 0)
                set = 1;
            sync_hard(set, request);
        }
        else if (strcmp("SENDER", mode) == 0 || strcmp("RECEIVER", mode) == 0)
        {
            enum P2P_Side driver;
            if (strcmp(mode, "SENDER") == 0)
            {
                MPIPCL_DEBUG("SENDER INFO detected\n");
                driver = SENDER;
            }
            else
            {
                MPIPCL_DEBUG("RECEIVER INFO detected\n");
                driver = RECEIVER;
            }

            // if not driver spawn progress thread
            if (request->side != driver)
            {
                int t_res = pthread_create(&(request->sync_thread), NULL,
                                           threaded_sync_driver, (void*)request);
                assert(0 == t_res);
                request->threaded = RUNNING;
                return MPI_SUCCESS;
            }
            else
            {
                sync_side(driver, request);
            }
        }
        else
        {
            // default to bulk request
            MPIPCL_DEBUG("DEFAULT INFO Object detected\n");
            sync_hard(1, request);
        }
    }

    MPIPCL_DEBUG("%d: internal setup!\n", request->side);
    // finish setup after sync function finishes.
    internal_setup(request);

    return MPI_SUCCESS;
}

// setup internal requests and status control variables for request -- always
// needed. Assumes that sync has been run and request->parts & request->size has
// been set.
void internal_setup(MPIP_Request* request)
{
    MPI_Aint lb, extent, offset;
    meta* mes = request->comm_data;

    // get data_type from meta data.
    int ret_val = MPI_Type_get_extent(mes->type, &lb, &extent);
    assert(MPI_SUCCESS == ret_val);

    // create internal request array
    request->request = (MPI_Request*)malloc(request->parts * sizeof(MPI_Request));
    assert(request->request != NULL);

    // create partition status arrays.
    request->internal_status = (atomic_int*)malloc(sizeof(atomic_int) * request->parts);
    assert(request->internal_status != NULL);
    
    request->complete = (bool*)malloc(sizeof(bool) * request->parts);
    assert(request->complete != NULL);

    // for each allocated partition create a request based on side.
    for (int i = 0; i < request->parts; i++)
    {
        // calculate offsets and setup internal requests
        offset = i * request->size * extent;
        if (request->side == SENDER)
        {
            ret_val = MPI_Send_init((char*)mes->buff + offset, request->size, mes->type,
                                    mes->partner, mes->tag + i, mes->comm,
                                    &request->request[i]);
            assert(MPI_SUCCESS == ret_val);
            MPIPCL_DEBUG("Send_init called - buffer: %p - req pointer: %p\n",
                         (void*)((char*)mes->buff + offset), (void*)&request->request[i]);
        }
        else
        {
            ret_val = MPI_Recv_init((char*)mes->buff + offset, request->size, mes->type,
                                    mes->partner, mes->tag + i, mes->comm,
                                    &request->request[i]);
            assert(MPI_SUCCESS == ret_val);
            MPIPCL_DEBUG("Recv_init called - buffer: %p - req pointers: %p\n",
                         (void*)(void*)((char*)mes->buff + offset),
                         (void*)&request->request[i]);
        }

        // Since they're atomic, might as well do the atomic init call
        atomic_init(&request->internal_status[i], 0);
        request->complete[i] = 0;
    }
}

// initialize progress status in request.
void reset_status(MPIP_Request* request)
{
    // reset internal flags
    for (int i = 0; i < request->parts; i++)
    {
        request->internal_status[i] = 0;
        request->complete[i]        = 0;
    }
    for (int i = 0; i < request->local_parts; i++)
    {
        request->local_status[i] = 0;
    }
}

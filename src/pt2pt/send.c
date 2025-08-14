#include <math.h>

#include "mpipcl.h"

// call send function on all marked partitions
// catchup function for sync thread.
void send_ready(MPIP_Request* request)
{
    // call send on each currently marked partition
    for (int i = 0; i < request->local_parts; i++)
    {
        if (request->local_status[i])
        {
            general_send(i, request);
        }
    }
}

static inline void map_local_to_network_partitions(int user_partition_id,
                                                   MPIP_Request* request,
                                                   int* start,
                                                   int* end)
{
    int user_partitions    = request->local_parts;
    int network_partitions = request->parts;

    int temp_start = (network_partitions / user_partitions) * user_partition_id;
    int temp_end   = ceil(network_partitions / user_partitions) * (user_partition_id + 1);

    MPIPCL_DEBUG(
        "Partition %d - Start %d End %d \n", user_partition_id, temp_start, temp_end);
    assert(temp_start >= 0 && temp_end > temp_start && temp_end <= network_partitions);

    *start = temp_start;
    *end   = temp_end;
}

// given partition id, send now ready internal requests.
void general_send(int id, MPIP_Request* request)
{
    int start_part = 0;
    int end_part   = 0;
    int threshold  = 0;
    MPIPCL_DEBUG("User Partitions %d - Network Partitions %d\n",
                 request->local_parts,
                 request->parts);
    if (request->local_parts <= request->parts)
    {
        map_local_to_network_partitions(id, request, &start_part, &end_part);
        threshold = 1;
    }
    else /* Must be: if(request->local_parts > request->parts) */
    {
        threshold  = request->local_parts / request->parts;
        start_part = id / threshold;
        end_part   = start_part + 1;
    }

    MPIPCL_DEBUG("User Partition %d - Start %d End %d \n", id, start_part, end_part);

    // for each internal request affected
    for (int i = start_part; i < end_part; i++)
    {
        // increase the number of "local" partitions ready (using atomics)
        int prior_value = atomic_fetch_add_explicit(
            &request->internal_status[i], 1, memory_order_relaxed);

        MPIPCL_DEBUG("Network Partition %d - Count: %d, Threshold: %d\n",
                     i,
                     prior_value + 1,
                     threshold);

        // if the number of local partitions needed for one network partition
        // are ready
        if (prior_value + 1 == threshold)
        {
            // start associated request
            MPIPCL_DEBUG("Starting request %d %p \n", i, (void*)(&request->request[i]));
            int ret_val = MPI_Start(&request->request[i]);
            assert(MPI_SUCCESS == ret_val);
        }
    }
}

// maps recv_buffer offset -- parried related.
// returns 1 if ready, 0 otherwise.
// all results from race condition identical, no lock needed.
int map_recv_buffer(int id, MPIP_Request* request)
{
    if (request->local_status[id] == true)
    {
        return 1;
    }

    int start = 0;
    int end   = 0;
    if (request->local_parts <= request->parts)
    {
        map_local_to_network_partitions(id, request, &start, &end);
    }
    else
    {
        int threshold = request->local_parts / request->parts;
        start         = id / threshold;
        end           = start + 1;
    }

    // check status of dependent requests
    int flag = 0;
    MPIPCL_DEBUG("User Partitions %d - Network Partitions %d\n",
                 request->local_parts,
                 request->parts);
    MPIPCL_DEBUG("Checking Requests: [%d: %d)\n", start, end);
    int ret_val =
        MPI_Testall(end - start, &request->request[start], &flag, MPI_STATUSES_IGNORE);
    assert(MPI_SUCCESS == ret_val);

    // if true store for future shortcut
    request->local_status[id] = flag;

    return flag;
}

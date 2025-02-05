#include "mpipcl.h"

#include <math.h>

// call send function on all marked partitions
// catchup function for sync thread.
void send_ready(MPIPCL_REQUEST *request)
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

static inline void map_local_partition(int user_partition_id, MPIPCL_REQUEST* request, int* start, int* end)
{
	int user_partitions = request->local_parts;
	int network_partitions = request->parts;

	// MPIPCL_DEBUG("User %d Network %d \n",user_partitions, network_partitions);

	int temp_start = (network_partitions / user_partitions) * user_partition_id;
	int temp_end   = ceil(network_partitions / user_partitions) * user_partition_id+1;

	MPIPCL_DEBUG("Start %d End %d \n",temp_start, temp_end);
	assert(temp_start >= 0 && temp_end > temp_start && temp_end <= network_partitions);

	*start = temp_start;
	*end   = temp_end;
}

// given partition id, send now ready internal requests.
void general_send(int id, MPIPCL_REQUEST *request)
{
	int start_part, end_part;
	map_local_partition(id, request, &start_part, &end_part);

	// for each internal request effected
	for (int i = start_part; i < end_part; i++)
	{
		// increase the number of "local" partitions ready
		request->internal_status[i]++;

		int threshold = request->local_parts / request->parts;
		MPIPCL_DEBUG("Count: %d, Threshold: %d\n", request->internal_status[i], threshold);

		// if the number of local partitions needed for one network partition are ready
		if (request->internal_status[i] == threshold)
		{
			// start associated request
			MPIPCL_DEBUG("Starting request %d %p \n", i, (void *) (&request->request[i]));
			int ret_val = MPI_Start(&request->request[i]);
			assert(MPI_SUCCESS == ret_val);
		}
	}
}

// maps recv_buffer offset -- parried related.
// returns 1 if ready, 0 otherwise.
// all results from race condition identical, no lock needed.
int map_recv_buffer(int id, MPIPCL_REQUEST *request)
{
	if (request->local_status[id] == true)
	{
		return 1;
	}

	int start, end;
	map_local_partition(id, request, &start, &end);

	// check status of dependent requests
	int flag = 0;
	int ret_val = MPI_Testall(end - start, &request->request[start], &flag, MPI_STATUSES_IGNORE);
	assert(MPI_SUCCESS == ret_val);

	// if true store for future shortcut
	request->local_status[id] = flag;

	return flag;
}

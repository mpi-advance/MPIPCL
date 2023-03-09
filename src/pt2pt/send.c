#include "mpipcl.h"

// call send function on all marked partitions
// catchup function for sync thread.
void send_ready(MPIX_Request *request)
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

// given partition id, send now ready internal requests.
void general_send(int id, MPIX_Request *request)
{
	// pthread_mutex_lock(&request->lock);

	// update status of internal requests.
	map_send_buffer_count(id, request);

	// pthread_mutex_unlock(&request->lock);
}

// maps send_buffer offsets - advice on locking mechanism needed.
// Use percentile filling to determine
// Pros: does not have to check status external partitions
// Cons: Due to rounding error only works on up to 256ish parts.
void map_send_buffer_percent(int id, MPIX_Request *request)
{
	int start_element = id * request->local_size;
	int current_part = start_element / request->size;
	int end_part = (start_element + request->local_size - 1) / request->size;

	int start_offset = current_part * request->size; // first element of first i request.
	int remaining = request->local_size;

	int hanging = 0;
	// calculate how many elements are non-overlapping in initial request
	if (start_offset < start_element)
	{
		hanging = start_element - start_offset;
	}

	do
	{
		// if local completely fits inside calculate percentage, fill, and exit
		if (hanging + remaining <= request->size)
		{
			request->internal_status[current_part] += (double)remaining / request->size;
			return;
		}
		// calculate how much of the supplied fits in the first partition.
		else
		{
			int overlap = request->size - hanging;
			request->internal_status[current_part] += (double)(overlap) / request->size; // convert to avoid int math
			remaining = remaining - overlap;
		}

		// move on to next partition.
		current_part++;
		if (current_part > end_part)
			break;
		hanging = 0;

	} while (remaining > 0);
}

// Run simple boolean check
// Pros: Simple concept.
// Cons: Have to use nested for loop.
void map_send_buffer_bool(int id, MPIX_Request *request)
{
	int start_part = id * request->local_size / request->size;
	int end_part = ((id + 1) * request->local_size - 1) / request->size;

	// for each internal request effected
	for (int i = start_part; i <= end_part; i++)
	{
		// calculate the external partitions needed.
		int ex_start_part = i * request->size / request->local_size;
		int ex_end_part = ((i + 1) * request->size - 1) / request->local_size;
		bool ready = false;

		for (int j = ex_start_part; j <= ex_end_part; j++)
		{
			if (request->local_status[j] == false)
			{
				ready = false;
				break;
			}
			else
			{
				ready = true;
			}
		}

		// if ready is true after loop set status as ready.
		request->internal_status[i] = ready;
	}
}

// Run simple boolean check
// Pros: Simple concept.
// Cons: Have to use nested for loop.
void map_send_buffer_count(int id, MPIX_Request *request)
{
	// MPIPCL_DEBUG("AT MAP %d \n",request->internal_status[id]);
	int start_part = id * request->local_size / request->size;
	int end_part = ((id + 1) * request->local_size - 1) / request->size;

	// MPIPCL_DEBUG("start %d end %d \n",start_part, end_part);

	// for each internal request effected
	for (int i = start_part; i <= end_part; i++)
	{
		// calculate the external partitions needed.
		int ex_start_part = i * request->size / request->local_size;
		int ex_end_part = ((i + 1) * request->size - 1) / request->local_size;
		int threshold = ex_end_part - ex_start_part + 1;

		request->internal_status[i]++;

		// if ready is true after loop set status as ready.
		if (request->internal_status[i] == threshold)
		{
			MPIPCL_DEBUG("Starting request %d \n", i);
			int ret_val = MPI_Start(&request->request[i]);
			assert(MPI_SUCCESS == ret_val);
		}
	}
}

// maps recv_buffer offset -- parried related.
// returns 1 if ready, 0 otherwise.
// all results from race condition identical, no lock needed.
int map_recv_buffer(int id, MPIX_Request *request)
{
	if (request->local_status[id] == true)
	{
		return 1;
	}

	int start = id * request->local_size / request->size;
	int end = ((id + 1) * request->local_size - 1) / request->size;

	// check status of dependent requests
	int flag = 0;
	int ret_val = MPI_Testall(end - start + 1, &request->request[start], &flag, MPI_STATUSES_IGNORE);
	assert(MPI_SUCCESS == ret_val);

	// if true store for future shortcut
	request->local_status[id] = flag;

	return flag;
}

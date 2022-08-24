#include "mpipcl.h"
//implemented synchronization modes 

void sync_hard(int option, MPIX_Request* request)
{
	//check that option divides num elements equally. 
	//assert(request->local_parts*request->local_size)%option == 0);
	
	request->parts = option;
	request->size  = request->local_parts*request->local_size/option;
	
	//printf("%d : %d parts of size %d \n", request->side, request->parts, request->size);  
	
	return;
}

//Send/Receive sync data to/from partner thread
void sync_side(int driver, MPIX_Request* request)
{
	
	//extract comm data from meta block
	int partner = request->comm_data->partner;	
	MPI_Comm comm = request->comm_data->comm;
	MPI_Request* holder;
	//Package and send data if driver.
	int syncdata[2];
	if(request->side == driver){
	        syncdata[0] = request->local_parts;
		syncdata[1] = request->local_size;
		//printf("SENDING DRIVER DATA\n");
		//fflush(stdout);
		
		MPI_Send(syncdata, 2, MPI_INT, partner, 990, comm);
		
		//printf("DRIVER DATA SENT\n");
		//fflush(stdout);
	}
	else{
		MPI_Recv(syncdata, 2, MPI_INT, partner, 990, comm, MPI_STATUS_IGNORE);
		//printf("DRIVER DATA RECV\n");
		//fflush(stdout);
	}
	
	//copy data into proper location on request
	request->parts = syncdata[0];
	request->size  = syncdata[1];
	
	printf("%d : sync data recieved %d %d \n", request->side, request->parts, request->size);
	fflush(stdout);
	
	return;
}

//instructions for synchronization thread
//defaults to sync_side. 
void* threaded_sync_driver(void* args)
{
	//Convert blob to correct type
	MPIX_Request* request = (MPIX_Request*)args;
	
	sync_side(!request->side, request);
	internal_setup(request);

	//lock and check request status. 
	pthread_mutex_lock(&request->lock);
	  //if request is active(MPI_Start), run catchup tasks.
	  if(request->state != ACTIVE){
	      if(request->side == RECEIVER){assert(MPI_Startall(request->size, request->request) == MPI_SUCCESS);}
	      else {
	      request->send_catchup_fun=send_ready;
	      request->send_catchup_fun(request);
	      }
      }	
	  //once caught up, signal thread completion and return. 
	  request->threaded = 1;
	pthread_mutex_unlock(&request->lock);
	
	return NULL;
}

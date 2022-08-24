/* Initial implementation of partitioned communication API */

#include "mpipcl.h"


//Init functions - call function in setup.c
int MPIX_Psend_init(void *buf, int partitions, MPI_Count count,
		    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIX_Request *request){
				
  request->side = SENDER;  
  prep(buf, partitions, count, datatype, dest, tag, info, comm, request);
  sync_driver(info, request);
  
  return MPI_SUCCESS;
}

int MPIX_Precv_init(void *buf, int partitions, MPI_Count count,
		    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, 
		    MPIX_Request *request) {
  
  request->side = RECEIVER;
  prep(buf, partitions, count, datatype, dest, tag, info, comm, request);
  sync_driver(info, request);

  return MPI_SUCCESS;
}



//Other Pready functions DO NOT call basic Pready, 
//modifying MPIX_Pready will NOT propagate the changes. 
//calls functions in send.c
int MPIX_Pready(int partition, MPIX_Request *request) {
  
  //check for calling conditions
  assert(request->side==SENDER && request->state==ACTIVE);
  
  //set local status
  request->local_status[partition] = 1;
  
  //local request and check status
  pthread_mutex_lock(&request->lock);
    int status = request->threaded;
  pthread_mutex_unlock(&request->lock);
  
  //if sync complete - call send function 
  if(status != 0){ request->send_fun(partition, request); }
  //else{ printf("%d delayed", partition );}

  
	
  return MPI_SUCCESS;
}

int MPIX_Pready_range(int partition_low, int partition_high, MPIX_Request *request) {
  /* TODO: throw appropriate error mesage. */
  
 assert(request->local_status == 0 && request->side==SENDER && request->state==ACTIVE);
  pthread_mutex_lock(&request->lock);
    int status = request->threaded;
  pthread_mutex_unlock(&request->lock);
  
  //set local
  for(int i=partition_low; i<=partition_high; i++){request->local_status[i] = 1;}
  
   //if thread spawned & not complete return after signal 
  if(status != 0){ request->send_catchup_fun(request); }
 
  
  return MPI_SUCCESS;
  
}

int MPIX_Pready_list(int length, int array_of_partitions[], MPIX_Request *request) {
  assert(request->local_status == 0 && request->side==SENDER && request->state==ACTIVE);
  pthread_mutex_lock(&request->lock);
    int status = request->threaded;
  pthread_mutex_unlock(&request->lock);
  
  for (int i = 0; i < length; i++){ request->local_status[i] = 1; }
  
  //if thread spawned & not complete return after signal 
  if(status != 0){ request->send_catchup_fun(request); }
  
  return MPI_SUCCESS;
}

//calls functions from send.c
int MPIX_Parrived(MPIX_Request *request, int partition, int *flag) {

  assert(request->side!=SENDER && request->state==ACTIVE);
  
  pthread_mutex_lock(&request->lock);
    int status = request->threaded;
  pthread_mutex_unlock(&request->lock);

  //if not synced - return early;
  if (status == 0){*flag = 0; return MPI_SUCCESS;}
  
  //if 1 to 1 map use shortcut
  if (request->local_parts == request->parts)
  {
	  return MPI_Test(&request->request[partition], flag, MPI_STATUS_IGNORE);
  }
  
  //else use mapping function.
  *flag = map_recv_buffer(partition, request);
  
  return MPI_SUCCESS;
}


int MPIX_Start(MPIX_Request *request){
    
	pthread_mutex_lock(&request->lock);
    int status = request->threaded;
    request -> state = ACTIVE;
	pthread_mutex_unlock(&request->lock);
    
	//setup complete
	if(status != 0){
		//reset ready flags
		for(int i=0; i<request->parts;i++)
		{
		   request->internal_status[i] = 0;
		}
		reset_status(request);
		//if receiver start recv requests. 
		if (request->side == RECEIVER){
			assert(MPI_Startall(request->parts, request->request) == MPI_SUCCESS);
		}
	}
	
  return MPI_SUCCESS;
}

int MPIX_Startall(int count, MPIX_Request array_of_requests[]){

  int i;
  for (i = 0; i  < count; i++)
    assert(MPIX_Start(&array_of_requests[i]) == MPI_SUCCESS);

  return MPI_SUCCESS;
}


//Other Wait function calls basic test, modifying MPIX_Wait will propagate the changes. 
int MPIX_Wait(MPIX_Request *request, MPI_Status *status){

  //if(request == NULL || request -> state == INACTIVE) {return MPI_SUCCESS;}
  
  pthread_mutex_lock(&request->lock);
  int t_status = request->threaded;
  pthread_mutex_unlock(&request->lock);

   
  //if thread has not completed. 
  if(t_status == 0){
       //wait until thread completes and joins
       pthread_join(request->sync_thread, NULL);
  }
  
  
  //printf("%d at wait ! \n", request->side);
  
  /*
  while(request->side != SENDER && t_status == )
  {
    pthread_mutex_lock(&request->lock);
      t_status = request->threaded;
    pthread_mutex_unlock(&request->lock);
  }
  */
  /*
  for(int i = 0; i<request->parts; i++)
  {
     printf("%d at wait %d \n", request->side, i);
     MPI_Wait(&request->request[i], MPI_STATUS_IGNORE);
     printf("Wait CHECK %d %f \n",  i, *((double*)request->comm_data->buff+i));
 
  
  }
 */
  //once setup is complete, wait on all internal partitions.  
  assert(MPI_Waitall(request->parts, request->request, MPI_STATUSES_IGNORE) == MPI_SUCCESS);

  //set state to inactive. 
  request -> state = INACTIVE;

  return MPI_SUCCESS;
}

int MPIX_Waitall(int count, MPIX_Request array_of_requests[],
                MPI_Status array_of_statuses[]){
  int i;
  
  for (i = 0; i < count; i++) {

         assert(MPIX_Wait(&array_of_requests[i], MPI_STATUS_IGNORE) == MPI_SUCCESS);
  }
  /* NOTE: array of MPI_Status objects is not updated */

  return MPI_SUCCESS; 
}

int MPIX_Waitany(int count, MPIX_Request array_of_requests[],
                int *index, MPI_Status *status){

  int i, flag=0;
  while (!flag) {
    for (i = 0; i < count; i++) {
      assert(MPIX_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE) == MPI_SUCCESS);
      if (flag == 1) {
        *index = i;
        break;
      }
    }
  }
  /* NOTE: MPI_Status object is not updated */

  return MPI_SUCCESS; 
}

int MPIX_Waitsome(int incount, MPIX_Request array_of_requests[],
                 int *outcount, int array_of_indices[],
                 MPI_Status array_of_statuses[]){
  int j=0, flag = 0;

  *outcount = 0;
  while(*outcount < 1){
  	for(int i = 0; i<incount; i++){
		assert(MPIX_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE) == MPI_SUCCESS);
	  	if(flag == 1){
	  	     *outcount = *outcount+1;
	  	     array_of_indices[j] = i;
	  	     j++;
	  	}
	}
  }
  return MPI_SUCCESS; 
}



//Other test function calls basic test, modifying MPIX_Test will propagate the changes. 
int MPIX_Test(MPIX_Request *request, int *flag, MPI_Status *status){
 
  *flag = 0;
  
  if(request == NULL || request -> state == INACTIVE) {return MPI_SUCCESS;}
  
  pthread_mutex_lock(&request->lock);
  int t_status = request->threaded;
  pthread_mutex_unlock(&request->lock);
  
  //if not synced, return false
  if(t_status == 0){ *flag = 0; return MPI_SUCCESS;}
  
  //else test status of each request in communication. 
  assert(MPI_Testall(request->parts, request->request, flag, MPI_STATUSES_IGNORE) == MPI_SUCCESS);
  
  if (*flag == 1) request -> state = INACTIVE;

  /* NOTE: MPI_Status object is not updated */
  return MPI_SUCCESS; 
}

int MPIX_Testall(int count, MPIX_Request array_of_requests[],
                int *flag, MPI_Status array_of_statuses[]){
  int i, myflag;
  *flag = 1;
  for (i = 0; i < count; i++) {
     assert(MPIX_Test(&array_of_requests[i], &myflag, MPI_STATUS_IGNORE) == MPI_SUCCESS);
     *flag = *flag & myflag;
  }

  /* NOTE: array of MPI_Status objects is not updated */
  return MPI_SUCCESS; 
}

int MPIX_Testany(int count, MPIX_Request array_of_requests[], int *index, int *flag, MPI_Status *status){
  
	//for each MPIX_request in provided array
	for(int i=0; i<count; i++){
	   	assert(MPIX_Test(&array_of_requests[i], flag, MPI_STATUS_IGNORE) == MPI_SUCCESS);
	   	if(*flag == 1){
	   		*index = i;
	   		break;
	   	}
	}
  return MPI_SUCCESS;	
}

int MPIX_Testsome(int incount, MPIX_Request array_of_requests[],
                 int *outcount, int array_of_indices[],
                 MPI_Status array_of_statuses[]){
  
  int j=0, flag=0;
  *outcount = 0;
  for(int i=0; i<incount; i++){
	  assert(MPIX_Test(&array_of_requests[i], &flag, MPI_STATUS_IGNORE) == MPI_SUCCESS);
	  if (flag == 1){
		 array_of_indices[*outcount]=i;
		 *outcount = *outcount+1;
		 j++;
	  }
  }
  return MPI_SUCCESS; 
}


//cleanup function
int MPIX_Request_free(MPIX_Request *request){

	//clear internal array of requests
  	for (int i = 0; i < request->parts; i++){
		assert(MPI_Request_free(&request->request[i]) == MPI_SUCCESS);
	}
  
	//free internal request status buffers
	free(request->local_status);
	free(request->internal_status);
	free(request->complete);
  
    //free comm_data -- //could this be freed earlier? 
    free(request->comm_data);
  
    //clean up hanging threads and controls 
	pthread_mutex_destroy(&request->lock);
	

  return MPI_SUCCESS; 
}


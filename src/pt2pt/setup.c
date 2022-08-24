#include "mpipcl.h"

//functions used by both side of the communication. 
//called by Init function in mpipcl.c
//calls functions in sync.c 



//fill in default values and bundle message data
void prep(void *buf, int partitions, MPI_Count count, MPI_Datatype datatype, int opp, int tag, MPI_Info info, MPI_Comm comm, MPIX_Request *request)
{
  
  /* update partitioned request object with default values*/
  request -> state = INACTIVE;
  request -> local_size = count;
  request -> local_parts = partitions;
  request -> parts = 1; //default to 1
  request->threaded = -1;
  
  /*create & set local status buffer*/
  request -> local_status    = (bool*) malloc(sizeof(bool)*partitions); // >100 if external partition is ready 
    
  /*set overall request lock*/
  assert(pthread_mutex_init(&request->lock, NULL) == 0); 
  
  //Record mesage options for future request creation
  meta* mesage = (meta*)malloc(sizeof(meta));
  mesage->buff = buf;
  mesage->partner = opp;
  mesage->tag = tag;
  mesage->comm = comm;
  mesage->type = datatype;
  
  //attach mesage data to request object
  request->comm_data = mesage;
  
  //mutex needed for anything more than trivial mapping case. 
  pthread_mutex_init(&request->lock, NULL); //create mutex
  
  //printf("%d : PREP COMPLETE\n", request->side);
  //fflush(stdout);
  
  return;
}

//decode info object and call appropriate sync operation
int sync_driver(MPI_Info info, MPIX_Request* request)
{
  int flag;
  char mode[10];
  char option[10];
  
  //check for info object
  //MPI_Info info = request->comm_data->info; 
  
  if(info == MPI_INFO_NULL){
          printf("NULL INFO detected\n");
          fflush(stdout);
	  sync_hard(1, request);
  } else {
  
    //if exist get selected mode
    MPI_Info_get(info, "PMODE", TAGLENGTH, mode, &flag);
    
    //Change behavior based on key - update convert to ENUM?
    
    if(strcmp("HARD", mode)==0)
      {
	MPI_Info_get(info, "SET", 10, option, &flag);
	assert(flag == 1);
	//printf("HARD INFO detected\n");
	//fflush(stdout);
	int set = atoi(option);
	if (set <= 0) set = 1;
	sync_hard(set, request);
      }
    else if(strcmp("SENDER", mode)==0 || strcmp("RECEIVER", mode)){
      int driver;
      
      //printf("SENDER INFO detected\n");
      //fflush(stdout);
      
      if(strcmp(mode,"SENDER")==0){driver = SENDER;}else{driver = RECEIVER;}
      
      //if not driver spawn progress thread
      if(request->side != driver){
	request->threaded = pthread_create(&(request->sync_thread), NULL, threaded_sync_driver, request);
	assert(request->threaded == 0); //confirm thread spawn.
	return MPI_SUCCESS;
      }
      else{
	sync_side(driver,request);
      }
    }
    else{
      //default to bulk request
      printf("DEFAULT INFO Object detected\n");
      fflush(stdout);
      sync_hard(1, request);
    }
  }

  //finish setup after sync function finishes. 
  internal_setup(request);

  return MPI_SUCCESS;

}

//setup internal requests and status control variables for request -- always needed. 
//Assumes that sync has been run and request->parts & request->size has been set. 
void internal_setup(MPIX_Request* request)
{
	MPI_Aint lb, extent, offset;
	meta* mes = request->comm_data;
	
	
	
    request->request = (MPI_Request*)malloc(request->parts*sizeof (MPI_Request));
    assert(request -> request != NULL);
	
	//get data_type from meta data. 
    assert(MPI_Type_get_extent(mes->type, &lb, &extent) == MPI_SUCCESS);
	
	//create partition status arrays.
	request -> internal_status =   (int*) malloc(sizeof(int)*request->parts); //true if ready to send or received
	request -> complete       =   (bool*) malloc(sizeof(bool)*request->parts); //internal request has been started
	
	//for each allocated partition create a request based on side. 
	for (int i = 0; i < request->parts; i++) 
	{	
	   //calculate offsets and setup internal requests
       offset = i * request->size * extent;
       if(request->side == SENDER)
       {
		   assert(MPI_Send_init((char*)mes->buff + offset, request->size, mes->type, mes->partner,mes->tag+i, mes->comm, &request->request[i]) == MPI_SUCCESS);
		   //MPI_Send((char*)mes->buff + offset, request->size, mes->type, mes->partner,mes->tag+i, mes->comm);
   //printf("i:%d, value %f \n", i, *(double*)((char*)mes->buff + offset) );
       }
       else
       {
       	MPI_Request test;
         	assert(MPI_Recv_init((char*)mes->buff + offset, request->size, mes->type, mes->partner,mes->tag+i, mes->comm, &request->request[i]) == MPI_SUCCESS);
			//MPI_Recv((char*)mes->buff + offset, request->size, mes->type, mes->partner,mes->tag+i, mes->comm, MPI_STATUS_IGNORE);
       } 
       
       request->internal_status[i] = 0;
		request->complete[i] = 0;
	}
	
	//reset_status(request);
	
	//determine appropriate send function. 
	if(request->side == SENDER)
	{
	    if(request->local_parts == request->parts)
  	    {
		    //printf("USING SIMPLE\n");
		    request->send_fun = &simple_send;
  	    }
	    else
	    {
	    	//printf("USING GENERAL\n");
		request->send_fun = &general_send;
	    }
	}
	
	//printf("%d: setup %d parts of size %d \n", request->side, request->parts, request->size);
	
	
	return;
}

//initialize progress status in request. 
void reset_status(MPIX_Request* request)
{
	//printf("STATUS SET\n");
	//reset internal flags
	for (int i = 0; i < request->parts; i++)
	{
		request->internal_status[i] = 0;
		request->complete[i] = 0;
	}
	for (int i=0; i<request->local_parts; i++)
	{
		request->local_status[i]=0;
	
	}
	
}

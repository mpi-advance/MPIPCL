#include "mpipcl.h"

//call send function on all marked partitions
//catchup function for sync thread. 
void send_ready(MPIX_Request* request)
{
	
	//call send on each currently marked partition 
	for(int i = 0; i<request->local_parts; i++)
	{
		if(request->local_status[i]){request->send_fun(i, request);}
	}
	
	return;
}

//quick short cut if send side has a 1 to 1 map
//e.g. number of internal & external partitions are equal
void simple_send(int id, MPIX_Request* request)
{      
	MPI_Start(&request->request[id]);
	printf("SEND CHECK %d %f \n",  id, *((double*)request->comm_data->buff+id));
 
	return;
}

//advice/feedback on locking mechanism needed. 

//given partition id, send now ready internal requests. 
void general_send(int id, MPIX_Request* request)
{
	
	
    pthread_mutex_lock(&request->lock);
	
	//printf("%d in Lock \n Starting Status %d \n", id, request->internal_status[id]);

	//update status of internal requests.
	map_send_buffer_count(id, request);
    
    /*
	double threshold = 1 - (double)1/(request->parts+1);
    for(int i = 0; i<request->parts; i++)
	{
		//if not started and ready, send
		if(!request->complete[i] && request->internal_status[i] > threshold)
		{
		    MPI_Start(&request->request[i]);
			request->complete[i] = true;
		}
	}
    */
       // printf("%d exiting Lock \n Starting Status %d \n\n", id, request->internal_status[id]);
       // fflush(stdout);
        pthread_mutex_unlock(&request->lock);
}

//maps send_buffer offsets - advice on locking mechanism needed. 
//Use percentile filling to determine
//Pros: does not have to check status external partitions        
//Cons: Due to rounding error only works on up to 256ish parts. 
void map_send_buffer_percent(int id, MPIX_Request* request)
{
	int start_element = id*request->local_size;
	int current_part  = start_element/request->size;
	int end_part      = (start_element+request->local_size-1)/request->size;
	
	int start_offset = current_part*request->size; //first element of first i request. 
	int remaining    = request->local_size;
	
	int hanging = 0;
	//calculate how many elements are non-overlapping in initial request
	if( start_offset < start_element){
	   hanging  = start_element-start_offset;
	}
	
	do{
       //if local completely fits inside calculate percentage, fill, and exit  
	   if(hanging+remaining <= request->size)
	   {
	      request->internal_status[current_part] += (double)remaining/request->size;
		  return;
	   }
	   //calculate how much of the supplied fits in the first partition. 
	   else
	   {
	      int overlap = request->size - hanging; 
		  request->internal_status[current_part] += (double)(overlap)/request->size; //convert to avoid int math
		  remaining = remaining-overlap;
	   }
	   
	   //move on to next partition. 
	   current_part++;
	   if(current_part > end_part) break;
	   hanging = 0;
	   
	}while(remaining > 0);
	
	return;
}

//Run simple boolean check
//Pros: Simple concept. 
//Cons: Have to use nested for loop. 
void map_send_buffer_bool(int id, MPIX_Request* request)
{
	int start_part    = id*request->local_size/request->size;
	int end_part      = ((id+1)*request->local_size-1)/request->size;
	
	//for each internal request effected 
	for(int i = start_part; i<=end_part; i++)
	{
		//calculate the external partitions needed. 
		int ex_start_part = i*request->size / request->local_size;
		int ex_end_part  = ((i+1)*request->size-1)/request->local_size; 
		bool ready = false;
		
		for(int j = ex_start_part; j<= ex_end_part; j++)
		{
			if(request->local_status[j] == false)
			{
				ready = false;
				break;
			}
			else
			{
				ready = true;
			}
		}
		
		//if ready is true after loop set status as ready. 
		request->internal_status[i]=ready;
	}
	return;
}

//Run simple boolean check
//Pros: Simple concept. 
//Cons: Have to use nested for loop. 
void map_send_buffer_count(int id, MPIX_Request* request)
{

        //printf("AT MAP %d \n",request->internal_status[id]);
	int start_part    = id*request->local_size/request->size;
	int end_part      = ((id+1)*request->local_size-1)/request->size;
	
	//printf("start %d end %d \n",start_part, end_part);
	
	//for each internal request effected 
	for(int i = start_part; i<=end_part; i++)
	{
		//calculate the external partitions needed. 
		int ex_start_part = i*request->size / request->local_size;
		int ex_end_part  = ((i+1)*request->size-1)/request->local_size; 
		int theshold = ex_end_part - ex_start_part + 1;
		int count = 0;
		
		//printf("request %d status %d theshold %d\n", i, request->internal_status[i],theshold);
		
		request->internal_status[i]++;
		
		//printf("request %d status update %d theshold %d\n", i, request->internal_status[i],theshold);
		
		//if ready is true after loop set status as ready. 
		if (request->internal_status[i] == theshold)
		{
		        //printf("Starting request %d \n", i);
			assert(MPI_Start(&request->request[i])==MPI_SUCCESS);
		}
		
		
	}
	return;
}

//maps recv_buffer offset -- parried related.
//returns 1 if ready, 0 otherwise. 
//all results from race condition identical, no lock needed.  
int map_recv_buffer(int id, MPIX_Request* request)
{
	
	if(request->local_status[id] == true){return 1;}
	
	int start = id*request->local_size/request->size;
	int end   = ((id+1)*request->local_size-1)/request->size;
	
	//check status of dependent requests 
	int flag = 0;
	assert(MPI_Testall(end-start+1, &request->request[id], &flag, MPI_STATUSES_IGNORE)==MPI_SUCCESS);
	
	//if true store for future shortcut
        request->local_status[id] = flag;		

	return flag;	
}

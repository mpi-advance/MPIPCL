# Quick summary

# Building Instructions
### Prequisites
- MPI package (supporting MPI 3.0 or later)
- Cmake 3.17

### Building the Library
The MPIPCL library is a fairly simple cmake build:
```
mkdir <build_dir>
cd <build_dir>
cmake <options> ..
make
```

### Build Options
 - -DBUILD_DYNAMIC_LIBS (ON) : create a static library instead of a shared library
 - -DBUILD_EXAMPLES (OFF): Build some examples. Examples by default are in <build>/examples/BASIC
 - -DEXAMPLES_TO_BIN(OFF) : If building examples, place examples in <install_dir>/bin instead of in build directory 

### Using the Library
In order to use the library, you will need to make sure it is either included in the RPATH or the containing directory is added to LD_LIBRARY_PATH
and you will need to include the supplied MPIPCL.h.  

# Basic Library Operation
The library requires a basic ordering of functions calls to work as designed. The init functions must be called before anyother function. These functions setup the internal channels for communication between the processes. This process occurs on a background thread and can prevent progress until completion, however the main thread may continue uninterrupted. 

Then the generated requests must be activated with MPIP_Start. NO DATA is transfered at this stage. 

Each partition on the Sender must be marked as ready by one of the Pready functions. Once marked ready the partition will be queued to send once the init functions finish the setup. Calling a wait operation on the request will block until all partitions in that request have been sent. 

The reciever may start accepting data once the init and start functions are complete. The data is placed in the recieve buffer at an offset determined by the init functions. The arrival status of a particular partition can be determined using MPIP_Parrived. 

####  Sender-Side:
1. MPIP_Psend_init()
2. MPIP_Start()
3. MPIP_Pready()
4. MPIP_Wait()

#### Reciever-Side
1. MPIP_Precv_init()
1. MPIP_Start()
1. MPIP_PArrived()
1. MPIP_Wait()

# MPIPCL API

#### Partitioned Communication API
'''
- MPIP_Psend_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)
- MPIP_Prev_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)
- MPIP_Pready(int partition, MPIPCL_REQUEST* request)
- MPIP_Pready_range(int partition_low, int partition_high, MPIPCL_REQUEST* request)
- MPIP_Pready_list(int length, int array_of_partitions[], MPIPCL_REQUEST* request)
- MPIP_Parrived(MPIPCL_REQUEST* request, int partition, int* flag)
'''

#### Modified MPI Functions 
These functions are simiply MPIPCL overrides of standard MPI functions. They should be considered the same as their MPI counterparts with minimal functional alterations. 
- MPIP_Start(MPIPCL_REQUEST* request)
- MPIP_Startall(int count, MPIPCL_REQUEST array_of_requests[])
- MPIP_Wait(MPIPCL_REQUEST* request, MPI_Status* status)
- MPIP_Wailall(int count, MPIPCL_REQUEST array_of_requests[], MPI_Status array_of_statuses[])
- MPIP_Waitany(int count, MPIPCL_REQUEST array_of_requests[], int* index, MPI_Status* status)
- MPIP_Waitsome(int incount, MPIPCL_REQUEST array_of_requests[], int* outcount, int array_of_indices[],MPI_Status array_of_statuses[]);
- MPIP_Test(MPIPCL_REQUEST* request, int* flag, MPI_Status* status)
- MPIP_Testall(int count, MPIPCL_REQUEST array_of_requests[], int* flag, MPI_Status array_of_statuses[]
- MPIP_Testany(int count, MPIPCL_REQUEST array_of_requests[], int* index,int* flag, MPI_Status* status
- MPIP_Testsome(int incount, MPIPCL_REQUEST array_of_requests[],int* outcount, int array_of_indices[],MPI_Status array_of_statuses[])
MPIP_Request_free(MPIP_REQUEST* request)

### Partitioned API
- MPIP_Psend_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)
    - Description: Setup internal requests and partitions
    - Inputs
        - void* buf: Buffer containing data
        - int partitions: Number of partitions to divide the buffer between
        - MPI_Count count: Number of 
        - MPI_Datatype datatype: the datatype of the information to be received
        - int src: The rank the partitions originate. 
        - int tag:  A tag for the request
        - MPI_Comm comm: The communicator to be used. 
        - MPI_Info info: additional information to be used (see Doxygen page*)
    - Outputs
        - MPIPCL_REQUEST* request created

- MPIP_Precv_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)
    - Description: Setup internal requests for recieving partitions 
    - Inputs
        - void* buf: Buffer containing data
        - int partitions: Number of partitions to divide the buffer between
        - MPI_Count count: Number of 
        - MPI_Datatype datatype:  
        - int dest: The destination rank
        - int tag:  A tag for the request
        - MPI_Comm comm: The communicator to be used. 
        - MPI_Info info: additional information to be used (see Doxygen page*)
    - Outputs
        - MPIPCL_REQUEST* request created 


- MPIP_Pready(int partition, MPIPCL_REQUEST* request)
    - Description: Mark the supplied partition as ready for transfer. 
    The partition should not be modified after being marked. 
    - Inputs: 
        int partition: id of the partition to be marked
        MPIPCL_REQUEST* request: request containing the partition to be marked.        
    - Outputs

- MPIP_Pready_range(int partition_low, int partition_high, MPIPCL_REQUEST* request)
  - Description: Mark the partitions with ids between partition_low and partition_high (inclusive) as ready to send. . 
    The partitions should not be modified after being marked. 
    - Inputs: 
        int partition: id of the partition to be marked
        MPIPCL_REQUEST* request: request containing the partition to be marked.        
    - Outputs


- MPIP_Pready_list(int length, int array_of_partitions[], MPIPCL_REQUEST* request)
  - Description: Mark the partitions with ids listed in the array_of_partitions as ready to send.

    The partitions should not be modified after being marked. 
    - Inputs: 
        int length: the number of partitions in array_of_partitions
        int array_of_partitions[]: an array of the ids of partitions to be marked
        int partition: id of the partition to be marked
        MPIPCL_REQUEST* request: request containing the partition to be marked.        
    - Outputs


- MPIP_Parrived(MPIPCL_REQUEST* request, int partition, int* flag)
    
    - Inputs: 
        MPIPCL_REQUEST* request: request containing the partition to be marked.
        int partition: id of the partition to be checked
    - Output:
        - flag: returned TRUE if the partition has arrived, False otherwise. 

#### Classes and Structs
Information about classes and structs may be accessed by using Doxygen with the supplied DoxyFile


### Acknowlegments
This work has been partially funded by ...
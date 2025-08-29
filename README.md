# Overview
Library implementation of MPI-4 Partitioned Communication (currently using persistent point-to-point communication).

Partitioned Communication is the breaking of a single buffer of data into smaller chunks(partitions) that can be prepared and sent independent of each other. Each partition can be safely modified and sent by a individual thread. 

This is a component of the MPI-Advance Project!

# Building Instructions
### Prequisites
- MPI package (supporting MPI 3.0 or later)
- CMake 3.17

### Building the Library
The MPIPCL library is a fairly simple CMake build:
```
mkdir <build_dir>
cd <build_dir>
cmake <options> ..
make
```

### Build Options
 - `-DBUILD_SHARED_LIBS` (ON) : Builds a shared library instead of a static library
 - `-DBUILD_EXAMPLES` (OFF): Build some examples. Examples by default are in `<build>/examples/BASIC`
 - `-DEXAMPLES_TO_BIN` (OFF) : If building examples, will also install examples to `<install_dir>/bin` in addition to `<build>/examples/BASIC`

### Using the Library
In order to use the library, you will need to make sure it is either included in RPATH or the containing directory is added to LD_LIBRARY_PATH and you will need to include the supplied MPIPCL.h.  

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
2. MPIP_Start()
3. MPIP_PArrived()
4. MPIP_Wait()

### Modifying Internal library behavior. 
The internal behavior of the library can be modified by using the MPI_Info object supplied to the init functions. Please note that if modifying behavior using the MPI_Info object, the same object should be supplied to both sides of the communiction. Default behavior is to combine all partitions into a single message. 

Valid Key:Value pairs for the MPI_Info object.  
	PMODE: Controls the factor for determining how many internal message channels are setup. 
		- HARD: The number of messages is set to the value in MPI_INFO.SET
		- SENDER: The number of messages equals the number of external partitions at the sender.
		- RECIEVER: The number of messages equals the number of external partitions at the sender.
	
	SET: How many internal messages to group external partitions into. Only used if PMODE=HARD or if no MPI_Info object is supplied. 
		- Must be a positive integer greater than or equal to 1. 
 
# MPIPCL API

#### Partitioned Communication API
```c
- MPIP_Psend_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)
- MPIP_Precv_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)
- MPIP_Pready(int partition, MPIPCL_REQUEST* request)
- MPIP_Pready_range(int partition_low, int partition_high, MPIPCL_REQUEST* request)
- MPIP_Pready_list(int length, int array_of_partitions[], MPIPCL_REQUEST* request)
- MPIP_Parrived(MPIPCL_REQUEST* request, int partition, int* flag)
```

#### Modified MPI Functions 
These functions are simiply MPIPCL overrides of standard MPI functions. They should be considered the same as their MPI counterparts with minimal functional alterations. 
```c
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
- MPIP_Request_free(MPIP_REQUEST* request)
```
### Partitioned API
```
MPIP_Psend_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)```
    - Description: Setup internal requests and partitions. May spawn thread to   
	continue progress in background after return  
    - Inputs
        - void* buf             // Buffer containing data from all partitions
        - int partitions        // Number of partitions to divide the buffer between
        - MPI_Count count       // Number of 
        - MPI_Datatype datatype // the datatype of the information to be received
        - int src               // The rank from which the partitions originate. 
        - int tag               //  A tag for the request
        - MPI_Comm comm         // The communicator to be used. 
        - MPI_Info info         // additional information to be used (see Doxygen page*)
    - Outputs
        - MPIPCL_REQUEST*       //request created

```
- MPIP_Precv_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIPCL_REQUEST* request)```
    - Description: Setup internal requests for recieving partitions. May spawn thread to   
	continue progress in background after return  
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
        - MPIPCL_REQUEST* 	   //request created 

```
- MPIP_Pready(int partition, MPIPCL_REQUEST* request)
```
    - Description: Mark the supplied partition as ready for transfer. 
    The partition should not be modified after being marked. 
    - Inputs: 
        - int partition: id of the partition to be marked  
    - Input/Outputs
		- MPIPCL_REQUEST* request: request containing the partitions to be marked. Partitions will be marked as ready after return.

```
- MPIP_Pready_range(int partition_low, int partition_high, MPIPCL_REQUEST* request)
```
  - Description: Mark the partitions with ids between partition_low and partition_high (inclusive) as ready to send. . 
    The partitions should not be modified after being marked. 
    - Inputs: 
        - int partition: id of the partition to be marked
           
    - Input/Output
		- MPIPCL_REQUEST* request: request containing the partitions to be marked. Partitions will be marked as ready after return.     

```
- MPIP_Pready_list(int length, int array_of_partitions[], MPIPCL_REQUEST* request)
```
  - Description: Mark the partitions with the ids listed in the array_of_partitions as ready to send.

    The partitions should not be modified after being marked. 
    - Inputs: 
        - int length: the number of partitions in array_of_partitions
        - int array_of_partitions[]: an array of the ids of partitions to be marked
        - int partition: id of the partition to be marked
              
    - Input/Output
		- MPIPCL_REQUEST* request: request containing the partitions to be marked. Partitions will be marked as ready after return 

```
- MPIP_Parrived(MPIPCL_REQUEST* request, int partition, int* flag)
``` 
    - Inputs 
        - MPIPCL_REQUEST* request: request containing the partition to be marked.
        - int partition: id of the partition to be checked
    - Output:
        - flag: returned TRUE if the partition has arrived, False otherwise. 

#### Classes and Structs
Information about classes, structures, and internal functions may be accessed by using Doxygen with the supplied .DoxyFile (run `doxygen .Doxyfile` from the top level of this repo).

### Acknowlegments
This work has been partially funded by ...
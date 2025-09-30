# Overview
Library implementation of MPI-4 Partitioned Communication (currently using persistent point-to-point communication).

Partitioned Communication is the breaking of a single buffer of data into smaller chunks (partitions) that can be prepared and sent individually. Each partition can be safely modified and sent by a individual thread. 

This is a component of the MPI-Advance Project!

# Building Instructions
### Prerequisites
- MPI implementation supporting MPI 3.0+
- CMake 3.17+
- C11+

### Building the Library
The MPIPCL library is a fairly simple CMake build:
```bash
mkdir <build_dir>
cd <build_dir>
cmake <options> ..
make <options>
```

### CMake Build Options
 - `-DBUILD_SHARED_LIBS` (ON) : Builds a shared library instead of a static library
 - `-DBUILD_EXAMPLES` (OFF): Build some examples. Examples by default are in `<build>/examples/BASIC`
 - `-DBUILD_TESTS` (OFF) : Build ctests for correctness checking. After building tests can be run using ctest in the main build folder. 

### Using the Library
In order to use the library, you will need to make sure it is either included in RPATH or the containing directory is added to LD_LIBRARY_PATH and you will need to include the supplied `MPIPCL.h`.  

# Basic Library Operation
The library requires a basic ordering of functions calls to work as designed (an example is shown below). The init functions (`MPIP_<Psend/Precv>_init`) must be called first to create a partitioned request. These functions setup the internal channels for communication between the processes, using a background thread to perform any communication necessary to setup the requests. Next, the generated requests must be activated with `MPIP_Start`. NO DATA is transferred at this stage. 

Each partition on the sending side must be marked as ready by one of the Pready functions (`MPIP_Pready<_list/_range>`). Once marked ready, the partition will be queued to send once the thread from init functions finish the setup. Calling a `MPIP_Wait` operation on the request will block until all partitions in that request have been sent. 

The receiver may start accepting data once the init and start functions are complete. The data is placed in the receive buffer at an offset determined by the init functions. The arrival status of a particular partition can be determined using `MPIP_Parrived`.

####  Sender-Side:
1. MPIP_Psend_init()
2. MPIP_Start()
3. MPIP_Pready()
4. MPIP_Wait()

#### Reciever-Side
1. MPIP_Precv_init()
2. MPIP_Start()
3. MPIP_Parrived()
4. MPIP_Wait()

### Modifying Internal library behavior 
The internal partitioning behavior of the library can be modified by using the `MPI_Info` object supplied to the init functions. Please note that if modifying behavior using the `MPI_Info` object, the same key-value pair should be supplied to both sides of the communication. Default behavior is to combine all partitions into a single message. 

Valid key-value pairs for the MPI_Info object:  
- `PMODE`: Controls the factor for determining how many internal message channels are setup. 
    - `HARD`: The number of messages is set to the value in the `SET` key in the `MPI_Info` object.
    - `SENDER`: The number of messages equals the number of external partitions at the sender.
    - `RECEIVER`: The number of messages equals the number of external partitions at the sender.
- `SET`: How many internal messages to group external partitions into. Only used if `PMODE=HARD` or if no `MPI_Info` object is supplied. 
    - Must be a positive integer greater than or equal to 1. 
		
If `MPI_INFO_NULL` is provided, or the keys are not set, then the library defaults to `PMODE=HARD` and `SET=1`. This results in a single internal message containing all the partitions that will only be sent once all user-facing partitions (the number provided to the `MPIP_Psend_init` call) have been marked as ready.
 
# MPIPCL API

#### Partitioned Communication API
Below are a list of MPI extension APIs provided by this library (using the `MPIP` prefix instead of `MPIX`).
```c
- MPIP_Psend_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIP_Request* request)
- MPIP_Precv_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int src, int tag, MPI_Comm comm, MPI_Info info, MPIP_Request* request)
- MPIP_Pready(int partition, MPIP_Request* request)
- MPIP_Pready_range(int partition_low, int partition_high, MPIP_Request* request)
- MPIP_Pready_list(int length, int array_of_partitions[], MPIP_Request* request)
- MPIP_Parrived(MPIP_Request* request, int partition, int* flag)
```

#### Modified MPI Functions 
These functions are simply MPIPCL overrides of standard MPI functions. They should be considered the same as their MPI counterparts with minimal functional alterations. 
```c
- MPIP_Start(MPIP_Request* request)
- MPIP_Startall(int count, MPIP_Request array_of_requests[])
- MPIP_Wait(MPIP_Request* request, MPI_Status* status)
- MPIP_Wailall(int count, MPIP_Request array_of_requests[], MPI_Status array_of_statuses[])
- MPIP_Waitany(int count, MPIP_Request array_of_requests[], int* index, MPI_Status* status)
- MPIP_Waitsome(int incount, MPIP_Request array_of_requests[], int* outcount, int array_of_indices[],MPI_Status array_of_statuses[]);
- MPIP_Test(MPIP_Request* request, int* flag, MPI_Status* status)
- MPIP_Testall(int count, MPIP_Request array_of_requests[], int* flag, MPI_Status array_of_statuses[])
- MPIP_Testany(int count, MPIP_Request array_of_requests[], int* index,int* flag, MPI_Status* status)
- MPIP_Testsome(int incount, MPIP_Request array_of_requests[],int* outcount, int array_of_indices[],MPI_Status array_of_statuses[])
- MPIP_Request_free(MPIP_REQUEST* request)
```
### Partitioned API Documentation
```c
MPIP_Psend_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIP_Request* request)
```
- Description: Setup internal requests and partitions. May spawn thread to   
continue progress in background after return  
- Inputs
    - void* buf: Buffer containing data from all partitions
    - int partitions:  Number of partitions to divide the buffer between
    - MPI_Count count: Number of elements in each partition
    - MPI_Datatype datatype: The datatype of the information to be received
    - int src:  The rank from which the partitions originate. 
    - int tag:  A tag for the request
    - MPI_Comm comm: The communicator to be used. 
    - MPI_Info info: Additional information to be used to control behavior 
                    (see above)
- Outputs
    - MPIP_Request* request: The request being populated
- Return
    -`MPI_SUCCESS` if the operation completes successfully. 
```c
MPIP_Precv_init(void* buf, int partitions, MPI_Count count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Info info, MPIP_Request* request)
```
- Description: Setup internal requests for receiving partitions. May spawn thread to   
continue progress in background after return  
- Inputs:
    - void* buf: Buffer containing data from all partitions
    - int partitions:  Number of partitions to divide the buffer between
    - MPI_Count count: Number of elements in each partition
    - MPI_Datatype datatype: The datatype of the information to be received
    - int src:  The rank from which the partitions originate. 
    - int tag:  A tag for the request
    - MPI_Comm comm: The communicator to be used. 
    - MPI_Info info: Additional information to be used to control behavior 
                    (see above)
- Outputs
    - MPIP_Request* request: The request being populated
- Return
    - `MPI_SUCCESS` if the operation completes successfully. 
```c
MPIP_Pready(int partition, MPIP_Request* request)
```
- Description: Mark the supplied partition as ready for transfer. 
The partition should not be modified after being marked. 
- Inputs: 
    - int partition: Id of the partition to be marked  
- Input/Outputs
    - MPIP_Request* request: Request containing the partitions to be marked. Partitions will be marked as ready after return.
- Return
    - `MPI_SUCCESS` if the operation completes successfully. 
```c
MPIP_Pready_range(int partition_low, int partition_high, MPIP_Request* request)
```
- Description: Mark the partitions with ids between partition_low and partition_high (inclusive) as ready to send.
The partitions should not be modified after being marked. 
- Inputs: 
    - int partition: Id of the partition to be marked
- Input/Output
    - MPIP_Request* request: Request containing the partitions to be marked. Partitions will be marked as ready after return.     
- Return
    - `MPI_SUCCESS` if the operation completes successfully.
```c
MPIP_Pready_list(int length, int array_of_partitions[], MPIP_Request* request)
```
- Description: Mark the partitions with the ids listed in the array_of_partitions as ready to send.
The partitions should not be modified after being marked. 
- Inputs: 
    - int length: The number of partitions in array_of_partitions
    - int array_of_partitions[]: An array of the ids of partitions to be marked
    - int partition: Id of the partition to be marked   
- Input/Output
    - MPIP_Request* request: Request containing the partitions to be marked. Partitions will be marked as ready after return 
- Return
    - `MPI_SUCCESS` if the operation completes successfully. 
```c
MPIP_Parrived(MPIP_Request* request, int partition, int* flag)
``` 
- Description: Sets flag to true if partitioned with the supplied id has arrived and is ready to use. 
- Inputs 
    - MPIP_Request* request: Request containing the partition to be marked.
    - int partition: Id of the partition to be checked
- Output:
    - flag: Returned True if the partition has arrived, False otherwise. 
- Return
    - `MPI_SUCCESS` if the operation completes successfully.
		
#### Classes and Structs
Information about classes, structures, and internal functions may be accessed by using Doxygen with the supplied .Doxyfile (run `doxygen .Doxyfile` from the top level of this repo).

### Acknowledgments
This work has been partially funded by ...

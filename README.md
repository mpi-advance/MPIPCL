# MPIPCL
Library implementation of MPI-4 Partitioned Communication (currently using persistent point-to-point communication).

This is a component of the MPI-Advance Project!

The API provided by this project can be found in `mpipcl.h` in `/include/`. The source code is in the `/src/pt2pt` folder. The current version of our library creates an individual MPI Request for each partition, that is stored in the `MPIA_Request` object. These partition requests are launched with the respective calls to `MPI_PREADY` (or its variants) on the send-side. On the receive-side, the partition requests are launched when the `MPIA_Request` is started. The current version of the library also features a progress thread in the background to synchronize the number of internal partitions to use (useful for when the send-side and receive-side partitions don't match).

The current implementation makes the following assumptions/shortcuts:
1) The datatype is the same on both the sender and receiver processes
2) Only contiguous datatype is supported
3) The amount of data described on the send-side must match the amount of data described on the receive-side, even if the partitions do not match
4) The status object is not updated; it is ignored
5) These API are invoked in MPI_THREAD_SERIALIZED mode (i.e., MPI_Init_thread with thread support for MPI_THREAD_SERIALIZED is used instead of MPI_Init)
6) A new MPI request object called MPIA_Request is used to support partitioned communication


## Building
The MPIPCL library is a fairly simple cmake build:

```
mkdir <build_dir>
cd <build_dir>
cmake <options> ..
make
```
A loaded MPI package is required, along with CMake version 3.17 or higher. CMake should detect the MPI and pick the right compiler, but always double check the output to make sure.

By default cmake creates a shared library in the build folder, to instead create a static library run  option '-DBUILD_DYNAMIC_LIBS=OFF'

## Examples
There are six examples provided by this library (found in the `examples/Basic` folder) Each example has the specific execution instructions at the top, but most follow this pattern:
`mpirun -np <num_processes> ./<test_executable> <npartitions> <bufsize>`

The first, second, and sixth example programs require only two processes, while the third, fourth, and fifth example programs require five processes. Note that `bufsize % npartitions == 0` must be true for all examples and combinations of buffer sizes and number of partitions. The fifth test is testing all of the `MPIA_Wait\Test` functions and their variants.

If `-DBUILD_EXAMPLES=ON` is provided during the `cmake` command, the tests will also be built when building MPIPCL. 

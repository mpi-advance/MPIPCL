# MPIPCL
Library implementation of MPI-4 Partitioned Communication (currently using persistent point-to-point communication).

This is a component of the MPI-Advance Project!

The API provided by this project can be found in `mpipcl.h` in `/include/`. Older versions of the header are also in this folder for reference. The source code is in the `/src/` folder, with the newest version being `mpipcl_v4.c`. This version of our library creates an individual MPI Request for each partition, that is stored in the `MPIX_Request` object. These partition requests are launched with the respective calls to `MPI_PREADY` (or its variants) on the send-side. On the receive-side, the partition requests are launched when the `MPIX_Request` is started. The latest version of the library features a progress thread in the background to synchronize the number of internal partitions to use (useful for when the send-side and receive-side partitions don't match).

Version 1 of this library features the same as version 4, but without the extra thread in the background, so partitions must match in the MPI calls. Version 0 is the simplest version, and collapses all of the partitions into a single request, and does not send anything until all partitions are done. We do not recommend using this version. The Makefile builds using Version 4.

The current implementation makes the following assumptions/shortcuts:
1) The datatype is the same on both the sender and receiver processes
2) Only contiguous datatype is supported
3) The amount of data described on the send-side must match the amount of data described on the receive-side, even if the partitions do not match
4) The status object is not updated; it is ignored
5) These API are invoked in MPI_THREAD_SERIALIZED mode (i.e., MPI_Init_thread with thread support for MPI_THREAD_SERIALIZED is used instead of MPI_Init)
6) A new MPI request object called MPIX_Request is used to support partitioned communication

To build the project, simply type `make` in the main directory.


## Examples
There are five examples provided by this library. Each example has the specific execution instructions at the top, but most follow this pattern:
`mpirun -np <num_processes> ./<test_executable> <npartitions> <bufsize>`

The first two example programs require only two processes, while the third, fourth, and fifth example programs require five processes. Note that `bufsize % npartitions == 0` must be true for all examples and combinations of buffer sizes and number of partitions. The fifth test is testing all of the `MPIX_Wait\Test` functions and their variants.

To build the examples, type `make  tests` to make all tests executables. To see specific test instructions, please look at the test file you want to run.

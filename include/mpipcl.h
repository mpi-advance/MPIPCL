/**
* @file mpipcl.h
* @file setup.c
* @file send.c
* @file sync.c
* @file mpipcl.c
* This file contains the definitions for the MPIP_Request object, 
* as well as the Partitioned Communication API and
* supporting functions
*/

/* Prototype and definitions related to partitioned communication APIs */
#ifndef __MPIPCL__
#define __MPIPCL__

#ifdef __cplusplus
extern "C" {
#endif

#include <mpi.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
#include <stdatomic.h>
#endif
#include <assert.h>

#define MPIPCL_TAG_LENGTH 20

// structure to hold message settings if threaded sync is necessary
/**
 * Structure to hold message settings if additional synchronization is necessary.
 * Contains necessary information to set up a two-sided communication channel.
 */
typedef struct _meta_
{
    void* buff;
    int partner;
    int tag;
    MPI_Comm comm;
    MPI_Datatype type;
} meta;

// enums for MPIP request attributes
/** 
 * Allowed values for stating which side 
 * of the communication the MPIP request 
 * is monitoring. 
 */
enum P2P_Side
{
    SENDER   = 0, ///< The request object is held by the sender.
    RECEIVER = 1  ///< The request object is held by the receiver. 
};

/**
 * Allowed activation states of the MPIP_request
 */
enum Activation
{
    INACTIVE = 0, ///< The request is inactive, data may not be transmitted.  
    ACTIVE   = 1  ///< The request is active, data transmission may be permitted. 
};

/**
 * Status of internal progress thread for asynchronous setup
 */
enum Thread_Status
{
    NONE     = -1, //!< The thread does not exist or 
    RUNNING  = 0,  //!< The thread has been created and is running setup tasks 
    FINISHED = 1   //!< The thread has completed the setup and has terminated. 
};

/**
 \class _MPIP_Request
 * the MPIP_Request object used in a majority of the MPIPCL API calls
 * Contains a an array of internal MPI_Requests for tracking progress of partition
 * transmission Note: local_parts * local_size should equal parts*size
 */
typedef struct _MPIP_Request
{

    enum Activation state;     //!< Activation state of the request.

    enum P2P_Side side;        //!< Side of the communication the request is providing.

    /** @brief
     * Array of booleans representing the ready status of each external partition.
     * If the data is marked as ready be sent, then the status is 1 else 0.
     */
    bool* local_status;  //! status array - true if external partition is ready

    /**	@brief
     * Array of booleans representing the ready status of each internal message.
     * If the status is 1, then the partition is ready to be sent. 
	 * A internal message should only be marked as 1 if all external partitions,
	 * that it depends on are marked as ready. 
     * Type depends on the language used during compilation atomic_int* is used for C
     * compilers. Void* is used for C++ compilers as atomic_int is not defined in C++.
     */
#ifdef __cplusplus
    void* internal_status;  // C++ can't use "atomic_int" from C, so let's just
                            // make it void *
#else
    atomic_int* internal_status;  // status array - true if internal partition is ready
#endif

    /** @brief
     * Booleans representing the completion status of the overall request
     * If complete then value = 1, else 0.
     * Should only be 1 if all internal messages have been complete, 
     */
    bool* complete;

    int local_parts;  //!< number of partitions visible externally

    int local_size;  //!< number of items in each partitions

    int parts;  //!< number of internal requests to complete

    int size;  //!< number of items in each internal request

    /** @brief
     * Array of internal requests, one request per internal partition.
     * All internal requests must be complete for the MPIP_Request to be considered
     * complete.
     */
    MPI_Request* request;  // array of "size" internal requests to process

    /**
     * Structure containing message data to setup internal requests as part of the 
	 * init functions
     */
    struct _meta_* comm_data;

    /** @brief
     * Handle to thread enabling background progress and non-blocking behavior 
	 * of the MPIP_init functions. 
	 * No data will be transferred until the thread completes its setup and finishes.
     */
    pthread_t sync_thread;
    pthread_mutex_t lock; //!< Mutex to handle access to background progress thread
    enum Thread_Status threaded; //!< Variable to monitor status of progress thread.
} MPIP_Request;


 /**
 *  @defgroup user_api Partitioned Communication API
 *  User-facing Partitioned Communication API made available by the library. 
 */

//---------------------------------------------------------------------------------
/** @brief 
 * This function takes information about the starting buffer,
 * external partitions, and communication target and populates
 * the supplied MPIP_Request object with information necessary 
 * for it to setup the send side of the communication channel. 
 * Behavior of the internal setups including the number of 
 * internal messages is controlled through the MPI_Info object.  
 * @ingroup user_api
 * @param [in]  buf  The memory buffer where all the partitions are. 
 *                  the buffer is required to be contiguous. 
 * @param [in]  partitions The number of externally facing partitions that 
 *             \p buf will be separated into.  
 * @param [in]  count      The number of elements inside \p buf
 * @param [in]  datatype   The datatype of each element inside buf
 * @param [in]  dest       The rank of the process to receive the messages
 * @param [in]  tag        Integer tag to be used by the request, 
 * @param [in]  comm       MPI_communicator to be used for the messages.  
 * @param [in]  info       MPI_info object used to define the internal behavior
 *                         of the request object. 
 * @param [out] request pointer to MPIP_request object to be populated.  
 * \callergraph
 */
int MPIP_Psend_init(void* buf,
                    int partitions,
                    MPI_Count count,
                    MPI_Datatype datatype,
                    int dest,
                    int tag,
                    MPI_Comm comm,
                    MPI_Info info,
                    MPIP_Request* request);

/** @brief 
 * This function takes information about the starting buffer, external partitions,
 * and communication target and populates the supplied MPIP_Request object
 * with information necessary for it to setup the receiver side
 * of the communication channel. 
 * Behavior of the internal setups including the number of internal messages
 * is controlled through the MPI_Info object.  
 * @ingroup user_api
 * @param [in]  buf        The memory buffer where all the partitions are. 
 *                         The buffer is required to be contiguous. 
 * @param [in]  partitions The number of externally facing partitions that
 * \p buf will be separated into.  
 * @param [in]  count      The number of elements inside \p buf
 * @param [in]  datatype   The datatype of each element inside buf
 * @param [in]  dest       The rank of the process to receive the messages
 * @param [in]  tag        Integer tag to be used by the request,
 * @param [in]  comm       MPI_communicator to be used for the messages.  
 * @param [in]  info       MPI_info object used to define the internal behavior
 *                         of the request object. 
 * @param [out] request    pointer to MPIP_request object to be populated
 */
int MPIP_Precv_init(void* buf,
                    int partitions,
                    MPI_Count count,
                    MPI_Datatype datatype,
                    int dest,
                    int tag,
                    MPI_Comm comm,
                    MPI_Info info,
                    MPIP_Request* request);

/** @brief 
 * This function marks the partition with the given id
 * on the supplied request as ready to send. 
 * Can only be called by sending process when the request is active. 
 * If the channel has finished setting up, the function will call general_send() 
 * in an attempt to transfer as soon as possible. 
 * @ingroup user_api
 * @param [in]  void* buf The memory buffer where all the partitions will be hosted 
 *                        The buffer is required to be contiguous. 
 * @param [in]  partitions  the number of externally facing partitions inside of buf. 
 * @param [in]  count       The number of elements inside buf
 * @param [in]  datatype    The datatype of each element inside buf
 * @param [in]  src         The rank of the process to receive the transfer. 
 * @param [in]  tag         The base tag to be used by the MPIPCL request
 * @param [in]  comm        The MPI_communicator context to be used by the request. 
 * @param [in]  info        MPI_info object used to define the internal behavior
 *                           of the request object. 
 * @param [in, out] MPIP_Request* request request object to be populated. 
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Pready(int partition, MPIP_Request* request);

/** @brief 
 * This function marks the partitions with the given ids between partition_low
 * and partition_high inclusive. 
 * The function works by calling MPIP_Pready on each partition, 
 * including invoking general_send() after each successful mark. 
 * Can only be called by sending process when the request is active. 
 * @ingroup user_api
 * @param [in]  partition_low  the id of the first partition to mark as ready (inclusive)
 * @param [in]  partition_high the id of the last partition to mark as ready (inclusive)
 * @param [in, out] MPIP_Request* request request object to be populated. 
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Pready_range(int partition_low, int partition_high, MPIP_Request* request);

/** @brief 
 * This function marks the partitions with ids listed in array_of_partitions 
 * as ready. The function works by calling MPIP_Pready on each partition
 * matching the supplied id, including invoking general_send() after marking each. 
 * Can only be called by sending process when the request is active. 
 * @ingroup user_api
 * @param [in]  length the number of partition ids included in \p array_of_partitions
 * @param [in]  array_of_partitions An array of partition ids to be marked as ready. 
 * @param [in, out] MPIP_Request* request request object to be populated. 
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Pready_list(int length, int array_of_partitions[], MPIP_Request* request);

/** @brief 
 * This function checks to see if a partition has been sent.
 * @ingroup user_api 
 * @param [in]  MPIP_Request* The request containing the partition to be checked.
 * @param [in]  int partition The id of the partition to be checked. 
 * @param [out] flag   the result of the check, 1 if the partition has arrived
 * and is ready to be used, 0 otherwise. 
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Parrived(MPIP_Request* request, int partition, int* flag);

/**
 *  @defgroup mpi_mod Modified MPI functions. 
 *  Overrides of necessary MPI Calls to work with the new request object. 
 *	Should be considered and used the same as their unmodified counterparts
 *  with minimal changes to work with the MPIP_Request object.
 * @{ 
 */

int MPIP_Start(MPIP_Request* request);
int MPIP_Startall(int count, MPIP_Request array_of_requests[]);

int MPIP_Wait(MPIP_Request* request, MPI_Status* status);
int MPIP_Waitall(int count,
                 MPIP_Request array_of_requests[],
                 MPI_Status array_of_statuses[]);
int MPIP_Waitany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 MPI_Status* status);
int MPIP_Waitsome(int incount,
                  MPIP_Request array_of_requests[],
                  int* outcount,
                  int array_of_indices[],
                  MPI_Status array_of_statuses[]);

int MPIP_Test(MPIP_Request* request, int* flag, MPI_Status* status);
int MPIP_Testall(int count,
                 MPIP_Request array_of_requests[],
                 int* flag,
                 MPI_Status array_of_statuses[]);
int MPIP_Testany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 int* flag,
                 MPI_Status* status);
int MPIP_Testsome(int incount,
                  MPIP_Request array_of_requests[],
                  int* outcount,
                  int array_of_indices[],
                  MPI_Status array_of_statuses[]);

int MPIP_Request_free(MPIP_Request* request);

 /** @}*/

/** 
 * \defgroup internal Internal Functions
 * These functions are not used by the Partitioned API calls, 
 * but are not meant to be invoked directly. 
 */

/** 
 * \defgroup setup Internal setup functions
 * @ingroup internal  
 * These functions are called during the MPIP_Init functions to setup the internal
 * communication channels. The functions are implemented in setup.c
 * @{
 */

/** @brief 
 * This function takes information about the starting buffer, 
 * external partitions, and communication target and populates the supplied 
 * MPIP_Request object. 
 * @param [in]  buf,        The location of the buffer holding the partitioned data.
 * @param [in]  partitions, The number of external partitions to be created. 
 * @param [in]  count,      The number of elements in each partition 
 * @param [in]  datatype,   The datatype of the 
 * @param [in]  opp,        The rank of the remote process. 
 * @param [in]  tag,        An integer tag to be used with matching messages 
 * @param [in]  comm        The MPI Communicator to be used for messaging 
 * @param [in, out] request Pointer to the request being initialized
 */
void prep(void* buf,
          int partitions,
          MPI_Count count,
          MPI_Datatype datatype,
          int opp,
          int tag,
          MPI_Comm comm,
          MPIP_Request* request);

/** @brief 
 * This function calls a synchronization function based on the information 
 * provided by the supplied MPI_Info object. 
 * Depending on the options a thread may be spawned to maintain non-blocking behavior
 * of the main thread. 
 * The function's behavior is controlled by the PNUM and SET values of the MPI_INFO object
 * \n
 * If MPI_INFO:PMODE = "HARD" then hard_sync is called. \n
 * If MPI_INFO:PMODE = "RECEIVER" or "SENDER" then a thread is started 
 * to complete the build process and queue work to be done after setup is complete. \n
 * If no object is supplied, the function creates a Hard sync with a single message. \n
 * @param [in]  info MPI_Info object 
 * @param [in, out] request Pointer to the request being initialized
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */ 	  
int sync_driver(MPI_Info info, MPIP_Request* request);

/** @brief Completes the creation of the internal message channels after the final number 
 * of messages and partitions are determined. 
 * @param [in, out] request Pointer to the request being initialized
 */
void internal_setup(_MPIP_Request* request);

/**
	*Reset the ready status of all external partitions in the supplied request to 0
	*The contents of the /reflocal_status
*/


/** @brief 
 * Resets the completion and ready status of all partitions and internal messages
 * to unmarked and uncompleted, respectfully. 
 * @param [in, out] request Pointer to the request being initialized
*/
void reset_status(MPIP_Request* request);

/** @} */


/** 
 * \defgroup sync synchronization functions
 * @ingroup internal   
 * These functions are called during the MPIP_Init functions and run in the background
 * to complete the creation of the internal messages. 
 * These functions are implemented in sync.c
 * @{
 */

/** @brief 
 * This functions sets the number of internal creates to a static value determined
 * by option.
 * @param [in] option The number of internal messages to create.
 * @param [in, out] request Pointer to the request being initialized
 */
void sync_hard(int option, MPIP_Request* request);

/** @brief 
 * This function allows one side of the communication to determine the number of 
 * internal messages. 
 * The driving side sends the information to the opposite side. 
 * @param [in] driver  which side of the communication is in charge of determining
 *                     how many internal messages to generate. 
 * @param [in, out] request Pointer to the request being initialized
 */
void sync_side(enum P2P_Side driver, MPIP_Request* request);

/** @brief 
 * This function is run by the background thread, 
 * the process captures the information from the driving process. 
 * After setting up the request the function calls different functions depending 
 * on which side of the communication is running the thread. 
 * Receiver: if the request is active, it starts all the internal requests 
 * Sender:  if the request is active attempt to send all partition marked as ready.
 * @param [in, out] request MPIP_Request object being initialized
 */
void* threaded_sync_driver(void* args);

/** @} */

/** 
 * \defgroup sends Internal Messaging Functions
 * @ingroup internal 
 * These functions map the internal partitions to internal messages
 * and back and send internal messages which are not waiting in non-marked partitions. 
 * @{
 */

/** @brief 
 * This function is run by the background thread, 
 * and attempts to send all partitions that have been marked as ready. 
 * @param [in, out] request Pointer to the request being initialized
 */
void send_ready(MPIP_Request* request);

/** @brief 
 * This function attempts to send the given partition. 
 * The internal message has no external partitions that are not ready
 * then the message is sent. 
 * Otherwise, no message is sent.  
 * @param [in, out] request Pointer to the request being initialized
 */
void general_send(int id, MPIP_Request* request);

// remap functions
/** @brief 
 * This function maps which internal messages are necessary for 
 * an external partition on the receiver
 * to have completely arrived. 
 * Returns 1 if the partition is complete or 0 otherwise. 
 * @param [in] id The partition to be checked. 
 * @param [in, out] request the request containing the partition, 
 *                          updates local flag with result. 
 * @return 1 if partition has arrived, else 0. 
*/
int map_recv_buffer(int id, MPIP_Request* request);

/** @} */

// debug functions
#if defined(WITH_DEBUG)
#define MPIPCL_DEBUG(X, ...) printf(X, ##__VA_ARGS__);
#else
#define MPIPCL_DEBUG(X, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif

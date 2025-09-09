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

/** @brief The length used for MPI_Info-related strings. */
#define MPIPCL_TAG_LENGTH 20

/** @brief Internal structure to hold communication message details
 * @details
 * Structure to hold message settings for when additional synchronization is
 * necessary. Contains necessary information to set up a two-sided communication channel.
 */
typedef struct _meta_
{
    void* buff;         //!< The buffer address to use in the P2P communication
    int partner;        //!< Rank of the peer to communicate with
    int tag;            //!< Tag to use with the message.
    MPI_Comm comm;      //!< MPI Communicator to communicate the message over
    MPI_Datatype type;  //!< The MPI Datatype to use with the buffer
} meta;

/** @brief Values for denoting the role of the process in the communication exchange.
 * @details
 * This enum describes which side of the P2P communication process a given @ref
 * MPIP_Request will represent. The enum may also be used to decide which side of the
 * communication is responsible for determining the number of internal partitions to be
 * used.
 *
 * @sa sync_driver
 */
enum P2P_Side
{
    SENDER   = 0,  //!< The request object is held by the sender.
    RECEIVER = 1   //!< The request object is held by the receiver.
};

/** @brief Allowed activation states of the @ref MPIP_Request
 */
enum Activation
{
    INACTIVE = 0,  //!< The request is inactive, data may not be transmitted.
    ACTIVE   = 1   //!< The request is active, data transmission may be permitted.
};

/** @brief Status of internal progress thread for asynchronous setup
 */
enum Thread_Status
{
    NONE     = -1,  //!< The thread does not exist or has not been created yet.
    RUNNING  = 0,   //!< The thread has been created and is running setup tasks.
    FINISHED = 1    //!< The thread has completed the setup and has terminated.
};

/** @brief The user facing request object representing partitioned communications
 * @details
 * This object is used in a majority of the MPIPCL API calls.
 * Contains an array of internal MPI_Requests for tracking progress of partition
 * transmission. Note that the number of external partitions may not equal the number of
 * internal partitions, but the total number of bytes described by both will be equal.
 */
typedef struct _MPIP_Request
{
    /** @brief Activation state of the request. */
    enum Activation state;

    /** @brief Side of the communication the request is providing */
    enum P2P_Side side;

    /** @brief Array of booleans representing the ready status of each external partition.
     * @details
     * If the data is marked as ready be sent by the user, then the appropriate index in
     * this array is set to 1 (true). Otherwise, the value in this array will be 0
     * (false). This only tracks the status of external partitions, not the internal
     * partitions.
     */
    bool* local_status;

    /**	@brief  Array of booleans representing the ready status of each internal message.
     * @details
     * If an index in this array is set to 1, then the internal partition is ready to be
     * sent. A internal message should only be marked as 1 if all external partitions that
     * it depends on are marked as ready. Type depends on the language used during
     * compilation: `atomic_int*` is used for C compilers; `void*` is used for C++
     * compilers as `atomic_int` is not defined in C++.
     */
#ifdef __cplusplus
    void* internal_status;  // C++ can't use "atomic_int" from C, so let's just
                            // make it void *
#else
    atomic_int* internal_status;
#endif

    /** @brief Booleans representing the completion status of the overall request
     * @details
     * TO BE REMOVED
     */
    bool* complete;

    /** @brief Number of partitions visible externally
     * @details
     * This value is equal to the partitions number provided by the user in
     * MPIP_Psend_init or MPIP_Precv_init.
     */
    int local_parts;

    /** @brief Number of items in each external partition
     * @details
     * This value is equal to the MPI_Count provided by the user in MPIP_Psend_init or
     * MPIP_Precv_init.
     */
    int local_size;

    /** @brief Number of internal partitions used
     * @details
     * Represents how many underlying partitions there are, where each partition is
     * represented by one MPI_Send (or MPI_Recv). The number of internal partitions
     * is influenced by the MPI_Info object passed to the MPIP_Psend_init and
     * MPIP_Precv_init calls.
     */
    int parts;

    /** @brief Number of items in each internal partition request
     * @details
     * Note: @ref local_parts * @ref local_size should equal @ref parts * @ref size.
     */
    int size;

    /** @brief Array of internal requests, one MPI_Request per internal partition.
     * @details
     * All internal requests must be complete for the @ref MPIP_Request to be considered
     * complete. Allocated to be of size @ref parts upon request creation.
     */
    MPI_Request* request;

    /** @brief Member that holds the basic message data to be sent.
     * @details
     * The data inside this struct is used to pass appropriate offsets and information to
     * to the underlying MPI calls used to populate @ref request
     */
    struct _meta_* comm_data;

    /** @brief Background thread used to negotiate number of internal partitions.
     * @details
     * Handle to thread enabling background progress and non-blocking behavior
     * of the MPIP_init functions. No data from the user buffer provided to the
     * partitioned APIs will be transferred until the thread completes its setup and
     * finishes. After finishing the negotiation of partitions, the thread *may* also
     * start some of the internal partition requests if @ref MPIP_Pready has been called
     * before the thread finished negotiations.
     * @sa threaded_sync_driver
     */
    pthread_t sync_thread;

    /** @brief Mutex to handle access to background progress thread */
    pthread_mutex_t lock;

    /** @brief Variable to monitor status of progress thread */
    enum Thread_Status threaded;
} MPIP_Request;

/** @defgroup user_api Partitioned Communication API
 * @brief User-facing Partitioned Communication API made available by the library.
 */

//---------------------------------------------------------------------------------
/** @brief Create a persistent partitioned send request
 * @details
 * This function takes information about the starting buffer,
 * external partitions, and communication target and populates
 * the supplied @ref MPIP_Request object with information necessary
 * for it to setup the send side of the communication channel.
 * Behavior of the internal setups including the number of
 * internal messages is controlled through the MPI_Info object.
 *
 * @ingroup user_api
 * @param [in]  buf        The memory buffer where all the partitions are.
 *                         The buffer is required to be contiguous.
 * @param [in]  partitions The number of externally facing partitions that
 *                         \p buf will be separated into.
 * @param [in]  count      The number of elements inside \p buf
 * @param [in]  datatype   The datatype of each element inside buf
 * @param [in]  dest       The rank of the process to receive the messages
 * @param [in]  tag        Integer tag to be used by the request
 * @param [in]  comm       MPI communicator to be used for the messages.
 * @param [in]  info       MPI_Info object used to define the internal behavior of the
 *                         request object.
 * @param [out] request    Pointer to @ref MPIP_Request object to be populated.
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

/** @brief Create a persistent partitioned receive request
 * @details
 * This function takes information about the starting buffer, external partitions,
 * and communication target and populates the supplied  @ref MPIP_Request object
 * with information necessary for it to setup the receiver side
 * of the communication channel. Behavior of the internal setups including the number of
 * internal messages is controlled through the MPI_Info object.
 * @ingroup user_api
 * @param [in]  buf        The memory buffer where all the partitions are.
 *                         The buffer is required to be contiguous.
 * @param [in]  partitions The number of externally facing partitions that
 *                         \p buf will be separated into.
 * @param [in]  count      The number of elements inside \p buf
 * @param [in]  datatype   The datatype of each element inside buf
 * @param [in]  src        The rank of the process to receive the messages from
 * @param [in]  tag        Integer tag to be used by the request,
 * @param [in]  comm       MPI communicator to be used for the messages.
 * @param [in]  info       MPI_Info object used to define the internal behavior
 *                         of the request object.
 * @param [out] request    Pointer to @ref MPIP_Request object to be populated
 */
int MPIP_Precv_init(void* buf,
                    int partitions,
                    MPI_Count count,
                    MPI_Datatype datatype,
                    int src,
                    int tag,
                    MPI_Comm comm,
                    MPI_Info info,
                    MPIP_Request* request);

/** @brief Mark a partition in a partitioned send request as ready
 * @details
 * This function marks the partition with the given id on the supplied request as ready to
 * send. Can only be called by sending process when the request is active. If the channel
 * has finished setting up, the function will call general_send() in an attempt to
 * transfer as soon as possible.
 *
 * @ingroup user_api
 * @param [in]  partition   The id of the partition to mark as ready.
 * @param [in, out] request The @ref MPIP_Request object to mark the partitions on.
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Pready(int partition, MPIP_Request* request);

/** @brief Mark the partitions within the given range as ready.
 * @details
 * The function works by calling @ref MPIP_Pready on each partition.
 * Can only be called by sending process when the request is active.
 *
 * @ingroup user_api
 * @param [in] partition_low  The id of the first partition to mark as ready (inclusive)
 * @param [in] partition_high The id of the last partition to mark as ready (inclusive)
 * @param [in, out] request   @ref MPIP_Request object to mark the partitions on.
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Pready_range(int partition_low, int partition_high, MPIP_Request* request);

/** @brief Mark the partitions in the array as ready.
 * @details
 * This function marks the partitions with ids listed in array_of_partitions
 * as ready. The function works by calling @ref MPIP_Pready on each partition
 * matching the supplied id.
 * Can only be called by sending process when the request is active.
 *
 * @ingroup user_api
 * @param [in] length              The number of partition ids included in
 *                                 \p array_of_partitions
 * @param [in] array_of_partitions An array of partition ids to be marked as ready.
 * @param [in, out] request        @ref MPIP_Request object to be populated.
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Pready_list(int length, int array_of_partitions[], MPIP_Request* request);

/** @brief Check to see if a partition has arrived yet.
 * @details
 * Uses map_recv_buffer() to determine if \p partition has arrived. Has a shortcut to
 * return 0 for all partitions if MPIP_Request::sync_thread is still running.
 *
 * @ingroup user_api
 * @param [in]  request   @ref MPIP_Request containing the partition to be checked.
 * @param [in]  partition The id of the partition to be checked.
 * @param [out] flag      The result of the check; 1 if the partition has arrived
 *                        and is ready to be used, 0 otherwise.
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int MPIP_Parrived(MPIP_Request* request, int partition, int* flag);

/** @defgroup mpi_mod Modified MPI Functions
 * @brief Overrides of necessary MPI calls to work with the new request object.
 * @details
 * APIs in this group should be considered as, and used the same as, their unmodified MPI
 * counterparts (with minimal changes to work with the @ref MPIP_Request object).
 * @{
 */

/** @brief Analogous to MPI_Start for @ref MPIP_Request objects. */
int MPIP_Start(MPIP_Request* request);

/** @brief Analogous to MPI_Startall for @ref MPIP_Request objects. */
int MPIP_Startall(int count, MPIP_Request array_of_requests[]);

/** @brief Analogous to MPI_Wait for @ref MPIP_Request objects. */
int MPIP_Wait(MPIP_Request* request, MPI_Status* status);

/** @brief Analogous to MPI_Waitall for @ref MPIP_Request objects. */
int MPIP_Waitall(int count,
                 MPIP_Request array_of_requests[],
                 MPI_Status array_of_statuses[]);

/** @brief Analogous to MPI_Waitany for @ref MPIP_Request objects. */
int MPIP_Waitany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 MPI_Status* status);

/** @brief Analogous to MPI_Waitsome for @ref MPIP_Request objects. */
int MPIP_Waitsome(int incount,
                  MPIP_Request array_of_requests[],
                  int* outcount,
                  int array_of_indices[],
                  MPI_Status array_of_statuses[]);

/** @brief Analogous to MPI_Test for @ref MPIP_Request objects. */
int MPIP_Test(MPIP_Request* request, int* flag, MPI_Status* status);

/** @brief Analogous to MPI_Testall for @ref MPIP_Request objects. */
int MPIP_Testall(int count,
                 MPIP_Request array_of_requests[],
                 int* flag,
                 MPI_Status array_of_statuses[]);

/** @brief Analogous to MPI_Testany for @ref MPIP_Request objects. */
int MPIP_Testany(int count,
                 MPIP_Request array_of_requests[],
                 int* index,
                 int* flag,
                 MPI_Status* status);

/** @brief Analogous to MPI_Testsome for @ref MPIP_Request objects. */
int MPIP_Testsome(int incount,
                  MPIP_Request array_of_requests[],
                  int* outcount,
                  int array_of_indices[],
                  MPI_Status array_of_statuses[]);

/** @brief Analogous to MPI_Request_free for @ref MPIP_Request objects. */
int MPIP_Request_free(MPIP_Request* request);

/** @}*/

/** @defgroup internal Internal Functions
 * @brief Internal functions not meant to be called by the end user.
 * @details
 * These functions are used by the implementations of the Partitioned API calls,
 * but are not meant to be invoked directly from a user using MPIPCL.
 */

/** @defgroup setup Setup Functions
 * @ingroup internal
 * @brief All internal functions related to the setup of an @ref MPIP_Request object
 * @details
 * These functions are called during the MPIP_Init functions to setup the internal
 * communication channels. The functions are implemented in setup.c
 * @{
 */

/** @brief Initialize an @ref MPIP_Request object
 * @details
 * This function takes information about the starting buffer,
 * external partitions, and communication target and populates the supplied
 * 
 * @ref MPIP_Request object.
 * @param [in]  buf         The location of the buffer holding the data to be partitioned.
 * @param [in]  partitions  The number of external partitions to be created.
 * @param [in]  count       The number of elements in each partition.
 * @param [in]  datatype    The datatype of the elements in each partition.
 * @param [in]  opp         The rank of the peer process.
 * @param [in]  tag         An integer tag to be used with matching messages
 * @param [in]  comm        The MPI Communicator to be used for messaging
 * @param [in, out] request Pointer to the @ref MPIP_Request being initialized
 */
void prep(void* buf,
          int partitions,
          MPI_Count count,
          MPI_Datatype datatype,
          int opp,
          int tag,
          MPI_Comm comm,
          MPIP_Request* request);

/** @brief Determines the appropriate method for synchronizing the number of partitions
 * @details
 * This function calls a synchronization function based on the information
 * provided by the supplied MPI_Info object. Depending on the options a thread may be
 * spawned to maintain non-blocking behavior of the main thread. The function's behavior
 * is controlled by the PNUM and SET values of the MPI_Info object:
 * - If `MPI_INFO:PMODE` = `HARD` then sync_hard() is called.
 * - If `MPI_INFO:PMODE` = `RECEIVER` or `SENDER` then a thread is started to complete the
 * build process and queue work to be done after setup is complete
 * (threaded_sync_driver())
 *
 * If no object is supplied, the function creates a `HARD` sync with a single message.
 *
 * If not synchronizing with a thread, then internal_setup() is called to finish
 * initializing the remaining arrays inside the @ref MPIP_Request object.
 *
 * @param [in] info         MPI_Info object
 * @param [in, out] request Pointer to the @ref MPIP_Request being initialized
 * @return MPI_SUCCESS if operation completes or MPI_FAILURE otherwise
 */
int sync_driver(MPI_Info info, MPIP_Request* request);

/** @brief Allocates memory for internal request structures.
 * @details
 * Completes the creation of the following @ref MPIP_Request members after the final
 * number of messages and partitions are determined:
 * - MPIP_Request::request
 * - MPIP_Request::internal_status
 * - MPIP_Request::complete
 * @todo REMOVE COMPLETE
 *
 * @param [in, out] request Pointer to the @ref MPIP_Request being initialized
 */
void internal_setup(MPIP_Request* request);

/** @brief Reset members of an @ref MPIP_Request to be ready to send data again
 * @details
 * Reset the following members to zero in preparation of starting the request again:
 * - MPIP_Request::internal_status (internal partitions)
 * - MPIP_Request::local_status (external partitions)
 * - MPIP_Request::complete
 * @todo REMOVE COMPLETE
 *
 * @param [in, out] request Pointer to the @ref MPIP_Request being reset
 */
void reset_status(MPIP_Request* request);

/** @} */

/** \defgroup sync Synchronization Functions
 * @ingroup internal
 * @brief All functions related to synchronizing the number of internal partitions
 * @details
 * These functions are called during the MPIP_Init functions. These functions are
 * implemented in sync.c
 * @{
 */

/** @brief Synchronize partition number using a hard-coded value
 * @details
 * This functions sets the number of internal creates to the value passed to the function.
 * No MPI communication takes place in this function; users should take care to pass the
 * same value on both sides of the P2P communication.
 *
 * @param [in] option       The number of internal messages to create.
 * @param [in, out] request Pointer to the @ref MPIP_Request being initialized
 */
void sync_hard(int option, MPIP_Request* request);

/** @brief Synchronize partition number over MPI based on direction
 * @details
 * This function allows one side of the communication to determine the number of
 * internal partitions to uses. The driving side sends the information to the opposite
 * side using a single blocking MPI message. Users should take care to make sure both side
 * will provide the same @ref P2P_Side as input.
 *
 * @param [in] driver       Which side of the communication is in charge of determining
 *                          how many internal partitions to use.
 * @param [in, out] request Pointer to the @ref MPIP_Request being initialized
 */
void sync_side(enum P2P_Side driver, MPIP_Request* request);

/** @brief The thread function used to synchronize partition number asynchronously.
 * @details
 * This function is run by the background thread, and will call sync_side() and once that
 * returns, will call internal_setup(). If the passed @ref MPIP_Request has not been
 * started after these complete, the thread will exit. Note: One thread will be created
 * per
 * @ref MPIP_Request.
 *
 * If the request has been started, then additional functions may be called depending on
 * the side of the communication:
 * - Receiver: if the request is active, it starts all
 * the internal requests using MPI_Startall.
 * - Sender: if the request is active attempt to send all partitions
 * marked as ready (using send_ready())
 *
 * @param [in, out] args True type is `MPIP_Request*`, the request being initialized
 */
void* threaded_sync_driver(void* args);

/** @} */

/** \defgroup sends Internal Messaging Functions
 * @ingroup internal
 * @brief All functions related to managing internal partitions and their requests.
 * @details
 * These functions map the internal partitions to internal messages
 * and back as well as controlling the outflow of the actual data.
 *
 * @{
 */

/** @brief Send all internal partitions marked as ready.
 * @details This function is run by the background thread,
 * and attempts to send all partitions that have been marked as ready using
 * general_send().
 *
 * @sa threaded_sync_driver
 * @param [in, out] request Pointer to the @ref MPIP_Request being initialized
 *
 */
void send_ready(MPIP_Request* request);

/** @brief Send the internal partitions covered by the provided user partition
 * @details
 * This function attempts to send the given user partition by first mapping the specified
 * user partition the internal partition(s). Each internal partition has it's external
 * count increases. If there are any internal partitions that have hit their threshold
 * (aka the all the necessary external partitions are ready), then
 * they are sent out to the peer process. Otherwise, no messages are sent.
 *
 * @param [in] id The user-facing partition id to be "sent".
 * @param [in, out] request Pointer to the @ref MPIP_Request the partition belongs to.
 */
void general_send(int id, MPIP_Request* request);

/** @brief Checks to see if the given user partition has arrived.
 * @details
 * This function maps which internal messages are necessary for
 * an external partition on the receiver to have completely arrived.
 *
 * @param [in] id           The id of the partition to be checked.
 * @param [in, out] request The @ref MPIP_Request containing the partition,
 *                          updates MPIP_Request::local_status with result.
 * @return 1 if partition has arrived, else 0.
 */
int map_recv_buffer(int id, MPIP_Request* request);

/** @} */

// debug functions
/** @brief Marco for adding output in debug build.
 * @details
 * This macro is a wrapper for `printf`, and will be compiled out if the code is not
 * compiled with a debug build.
 * @param X The string to pass to `printf`
 * @param ... The remaining values to pass to `printf`.
 */
#if defined(WITH_DEBUG)
#define MPIPCL_DEBUG(X, ...) printf(X, ##__VA_ARGS__);
#else
#define MPIPCL_DEBUG(X, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif

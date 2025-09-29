 /**
 * @file MPIP_Request.h
 * This file contains the definitions for the MPIP_Request object,
 */

/* Prototype and definitions related to partitioned communication APIs */
#ifndef __MPIPREQUEST__
#define __MPIPREQUEST__

#ifdef __cplusplus
extern "C" {
#endif

#include <mpi.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __cplusplus
#include <stdatomic.h>
#endif


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

#ifdef __cplusplus
}
#endif


#endif
 /**
 * @file pt2pt.h
 * This file contains the internal functions to enable point to point
 * version of the library
 */

/* Prototype and definitions related to partitioned communication APIs */
#ifndef __PT2PT__
#define __PT2PT__

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
 */

/** @brief Initialize an @ref MPIP_Request object
 * @details
 * This function takes information about the starting buffer,
 * external partitions, and communication target and populates the supplied
 * @ref MPIP_Request object.
 * @ingroup setup
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
 * @ingroup setup
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
 *
 * @ingroup setup
 * @param [in, out] request Pointer to the @ref MPIP_Request being initialized
 */
void internal_setup(MPIP_Request* request);

/** @brief Reset members of an @ref MPIP_Request to be ready to send data again
 * @details
 * Reset the following members to zero in preparation of starting the request again:
 * - MPIP_Request::internal_status (internal partitions)
 * - MPIP_Request::local_status (external partitions)
 *
 * @ingroup setup
 * @param [in, out] request Pointer to the @ref MPIP_Request being reset
 */
void reset_status(MPIP_Request* request);



/** \defgroup sync Synchronization Functions
 * @ingroup internal
 * @brief All functions related to synchronizing the number of internal partitions
 * @details
 * These functions are called during the MPIP_Init functions. These functions are
 * implemented in sync.c
 */

/** @brief Synchronize partition number using a hard-coded value
 * @details
 * This functions sets the number of internal creates to the value passed to the function.
 * No MPI communication takes place in this function; users should take care to pass the
 * same value on both sides of the P2P communication.
 *
 * @ingroup sync
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
 * @ingroup sync
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
 * @ingroup sync
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
 */

/** @brief Send all internal partitions marked as ready.
 * @details This function is run by the background thread,
 * and attempts to send all partitions that have been marked as ready using
 * general_send().
 *
 * @sa threaded_sync_driver
 * @ingroup sends
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
 * @ingroup sends
 * @param [in] id The user-facing partition id to be "sent".
 * @param [in, out] request Pointer to the @ref MPIP_Request the partition belongs to.
 */
void general_send(int id, MPIP_Request* request);

/** @brief Checks to see if the given user partition has arrived.
 * @details
 * This function maps which internal messages are necessary for
 * an external partition on the receiver to have completely arrived.
 *
 * @ingroup sends
 * @param [in] id           The id of the partition to be checked.
 * @param [in, out] request The @ref MPIP_Request containing the partition,
 *                          updates MPIP_Request::local_status with result.
 * @return 1 if partition has arrived, else 0.
 */
int map_recv_buffer(int id, MPIP_Request* request);





#ifdef __cplusplus
}
#endif

#endif
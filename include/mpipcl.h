/**
 * @file mpipcl.h
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
#endif
#include <assert.h>
#include "MPIP_Request.h"

/** @brief The length used for MPI_Info-related strings. */
#define MPIPCL_TAG_LENGTH 20


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

#include "mpipcl.h"

#include <cassert>

#include "mpipcl_request.hpp"

#ifdef __cplusplus
extern "C" {
#endif

/* Initial implementation of partitioned communication API */

int MPIA_Psend_init(void* buf, int partitions, MPI_Count count,
                    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Info info, MPIA_Request* request)
{
    using namespace MPIAdvance::mpipcl;

    MPIPCLRequest* user_request = new MPIPCLRequest(
        P2PSide::SENDER, buf, partitions, count, datatype, dest, tag, comm);

    user_request->sync_driver(info);

    *request = new _MPIA_Request{user_request};

    return MPI_SUCCESS;
}

int MPIA_Precv_init(void* buf, int partitions, MPI_Count count,
                    MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
                    MPI_Info info, MPIA_Request* request)
{
    using namespace MPIAdvance::mpipcl;

    MPIPCLRequest* user_request = new MPIPCLRequest(
        P2PSide::RECEIVER, buf, partitions, count, datatype, dest, tag, comm);

    user_request->sync_driver(info);

    *request = new _MPIA_Request{user_request};

    return MPI_SUCCESS;
}

int MPIA_Pready(int partition, MPIA_Request* request)
{
    if (MPIAdvance::RequestType::MPIPCL !=
        (*request)->internal_request->getSafeType())
    {
        /* TODO */
        return -1;
    }

    using namespace MPIAdvance::mpipcl;
    MPIPCLRequest* mpipcl_request =
        (MPIPCLRequest*)((*request)->internal_request);
    mpipcl_request->pready(partition);

    return MPI_SUCCESS;
}

int MPIA_Pready_range(int partition_low, int partition_high,
                      MPIA_Request* request)
{
    for (int i = partition_low; i <= partition_high; i++)
    {
        int ret_val = MPIA_Pready(i, request);
        assert(MPI_SUCCESS == ret_val);
    }
    return MPI_SUCCESS;
}

int MPIA_Pready_list(int length, int array_of_partitions[],
                     MPIA_Request* request)
{
    for (int i = 0; i < length; i++)
    {
        int ret_val = MPIA_Pready(array_of_partitions[i], request);
        assert(MPI_SUCCESS == ret_val);
    }
    return MPI_SUCCESS;
}

// calls functions from send.c
int MPIA_Parrived(MPIA_Request* request, int partition, int* flag)
{
    if (MPIAdvance::RequestType::MPIPCL !=
        (*request)->internal_request->getSafeType())
    {
        /* TODO */
        return -1;
    }

    using namespace MPIAdvance::mpipcl;
    MPIPCLRequest* mpipcl_request =
        (MPIPCLRequest*)((*request)->internal_request);

    *flag = mpipcl_request->parrived(partition);

    return MPI_SUCCESS;
}

#ifdef __cplusplus
}
#endif
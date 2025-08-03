#include <atomic>
#include <cassert>
#include <cmath>
#include <cstring>
#include <thread>
#include <vector>

#include "MPIAdvance/base.hpp"

namespace MPIAdvance
{
namespace mpipcl
{
// structure to hold message settings if threaded sync is necessary
struct MessageData
{
    void*        buff;
    int          partner;
    int          tag;
    MPI_Comm     comm;
    MPI_Datatype type;
};

enum P2PSide
{
    SENDER   = 0,
    RECEIVER = 1
};

enum Activation
{
    INACTIVE = 0,
    ACTIVE   = 1
};

enum ThreadStatus
{
    NONE     = -1,
    RUNNING  = 0,
    FINISHED = 1
};

class MPIPCLRequest : public Request
{
public:
    MPIPCLRequest(P2PSide _side, void* buf, int partitions, MPI_Count count,
                  MPI_Datatype datatype, int opp, int tag, MPI_Comm comm)
        : Request(MPIAdvance::RequestType::MPIPCL),
          state(Activation::INACTIVE),
          side(_side),
          local_size(count),
          local_parts(partitions),
          parts(1),
          threaded(ThreadStatus::NONE),
          local_status(partitions, false),
          comm_data({buf, opp, tag, comm, datatype})
    {
        /*set overall request lock*/
        int ret_val = pthread_mutex_init(&lock, NULL);
        assert(0 == ret_val);
        MPIPCL_DEBUG("%d : PREP COMPLETE\n", side);
    }
    ~MPIPCLRequest()
    {
        // Join thread incase request was freed too early
        if(sync_thread.joinable())
        {
            sync_thread.join();
        }

        // clear internal array of requests
        for (int i = 0; i < parts; i++)
        {
            int ret_val = MPI_Request_free(&requests[i]);
            assert(MPI_SUCCESS == ret_val);
        }

        // clean up hanging threads and controls
        pthread_mutex_destroy(&lock);
    }

    void pready(int partition)
    {
        MPIPCL_DEBUG("INSIDE PREADY\n");
        // check for calling conditions
        assert(side == P2PSide::SENDER && state == Activation::ACTIVE);

        // set local status
        local_status[partition] = 1;

        // local request and check status
        pthread_mutex_lock(&lock);
        ThreadStatus thread_status = threaded;
        pthread_mutex_unlock(&lock);

        // if sync complete - call send function
        if (thread_status != ThreadStatus::RUNNING)
        {
            general_send(partition);
        }
        else
        {
            MPIPCL_DEBUG("%d delayed\n", partition);
        }
    }

    bool parrived(int partition)
    {
        assert(side != P2PSide::SENDER);

        pthread_mutex_lock(&lock);
        ThreadStatus status = threaded;
        pthread_mutex_unlock(&lock);

        // if not synced - return early;
        if (status == ThreadStatus::RUNNING)
        {
            return false;
        }
        // else use mapping function to check status
        return map_recv_buffer(partition);
    }

    void start() override
    {
        MPIPCL_DEBUG("MPIX START CALLED: %d\n", side);
        pthread_mutex_lock(&lock);
        ThreadStatus thread_status = threaded;
        state                      = Activation::ACTIVE;
        pthread_mutex_unlock(&lock);

        // setup complete
        if (thread_status != RUNNING)
        {
            // reset ready flags
            reset_status();
            // if receiver start recv requests.
            if (side == P2PSide::RECEIVER)
            {
                MPIPCL_DEBUG("USER THREAD IS STARTING RECV:%d\n", parts);
                int ret_val = MPI_Startall(parts, requests.data());
                assert(MPI_SUCCESS == ret_val);
            }
        }
    }
    void wait() override
    {
        MPIPCL_DEBUG("Inside MPIX Wait: %d \n", side);
        pthread_mutex_lock(&lock);
        ThreadStatus t_status = threaded;
        pthread_mutex_unlock(&lock);

        // if thread has not completed.
        if (t_status == ThreadStatus::RUNNING)
        {
            // wait until thread completes and joins
            sync_thread.join();
        }

        // once setup is complete, wait on all internal partitions.
        MPIPCL_DEBUG("Waiting on %d reqs at address %p\n", parts,
                     (void*)requests);
        int ret_val = MPI_Waitall(parts, requests.data(), MPI_STATUSES_IGNORE);
        assert(MPI_SUCCESS == ret_val);

        // set state to inactive.
        state = Activation::INACTIVE;
    }

    bool test() override
    {
        if (state == Activation::INACTIVE)
        {
            MPIPCL_DEBUG(
                "Early test exit due to null or inactive request: %p\n",
                (void*)this);
            return false;
        }

        pthread_mutex_lock(&lock);
        ThreadStatus t_status = threaded;
        pthread_mutex_unlock(&lock);

        // if not synced, return false
        if (t_status == 0)
        {
            MPIPCL_DEBUG("Early test exit due to not synched\n");
            return false;
        }

        // else test status of each request in communication.
        int flag = 0;
        int ret_val =
            MPI_Testall(parts, requests.data(), &flag, MPI_STATUSES_IGNORE);
        assert(MPI_SUCCESS == ret_val);

        if (flag == 1)
            state = INACTIVE;

        return flag;
    }

    void sync_driver(MPI_Info info)
    {
        // check for info object
        if (info == MPI_INFO_NULL)
        {
            MPIPCL_DEBUG("NULL INFO detected\n");
            sync_hard(1);
        }
        else
        {
            int  flag;
            char mode[10];
            char option[10];
            // if exist get selected mode
            MPI_Info_get(info, "PMODE", MPIPCL_TAG_LENGTH, mode, &flag);

            // Change behavior based on key - update convert to ENUM?
            if (strcmp("HARD", mode) == 0)
            {
                MPIPCL_DEBUG("HARD INFO detected\n");
                MPI_Info_get(info, "SET", MPIPCL_TAG_LENGTH, option, &flag);
                assert(flag == 1);

                int set = atoi(option);
                if (set <= 0)
                    set = 1;
                sync_hard(set);
            }
            else if (strcmp("SENDER", mode) == 0 ||
                     strcmp("RECEIVER", mode) == 0)
            {
                P2PSide driver;
                if (strcmp(mode, "SENDER") == 0)
                {
                    MPIPCL_DEBUG("SENDER INFO detected\n");
                    driver = SENDER;
                }
                else
                {
                    MPIPCL_DEBUG("RECEIVER INFO detected\n");
                    driver = RECEIVER;
                }

                // if not driver spawn progress thread
                if (side != driver)
                {
                    sync_thread =
                        std::thread(&MPIPCLRequest::threaded_sync_driver, this);
                    threaded = RUNNING;
                    return;
                }
                else
                {
                    sync_side(driver);
                }
            }
            else
            {
                // default to bulk request
                MPIPCL_DEBUG("DEFAULT INFO Object detected\n");
                sync_hard(1);
            }
        }

        MPIPCL_DEBUG("%d: internal setup!\n", request->side);
        // finish setup after sync function finishes.
        internal_setup();
    }

    // reset internal flags
    void reset_status()
    {
        for (int i = 0; i < parts; i++)
        {
            internal_status[i] = 0;
            complete[i]        = 0;
        }
        for (int i = 0; i < local_parts; i++)
        {
            local_status[i] = 0;
        }
    }

private:
    void sync_hard(int option)
    {
        parts = option;
        size  = local_parts * local_size / option;
    }

    void sync_side(P2PSide driver)
    {
        // extract comm data from meta block
        int      partner = comm_data.partner;
        MPI_Comm comm    = comm_data.comm;
        // Package and send data if driver.
        int syncdata[2];
        if (side == driver)
        {
            syncdata[0] = local_parts;
            syncdata[1] = local_size;
            MPI_Send(syncdata, 2, MPI_INT, partner, 990, comm);
        }
        else
        {
            MPI_Recv(syncdata, 2, MPI_INT, partner, 990, comm,
                     MPI_STATUS_IGNORE);
        }

        // copy data into proper location on request
        parts = syncdata[0];
        size  = syncdata[1];

        MPIPCL_DEBUG("%d : sync data received %d %d \n", side, parts, size);
    }
    void internal_setup()
    {
        MessageData& mes = comm_data;

        // get data_type from meta data.
        MPI_Aint lb, extent;
        int      ret_val = MPI_Type_get_extent(mes.type, &lb, &extent);
        assert(MPI_SUCCESS == ret_val);

        /* Create all vectors that depend on the number of partitions */
        internal_status = std::vector<std::atomic<int>>(parts);
        complete        = std::vector<bool>(parts);
        requests        = std::vector<MPI_Request>(parts);

        // for each allocated partition create a request based on side.
        for (int i = 0; i < parts; i++)
        {
            // calculate offsets and setup internal requests
            MPI_Aint offset = i * size * extent;
            if (side == P2PSide::SENDER)
            {
                ret_val = MPI_Send_init((char*)mes.buff + offset, size,
                                        mes.type, mes.partner, mes.tag + i,
                                        mes.comm, &requests[i]);
                assert(MPI_SUCCESS == ret_val);
                MPIPCL_DEBUG(
                    "Send_init called - buffer: %p - req pointer: %p\n",
                    (void*)((char*)mes.buff + offset), (void*)&requests[i]);
            }
            else
            {
                ret_val = MPI_Recv_init((char*)mes.buff + offset, size,
                                        mes.type, mes.partner, mes.tag + i,
                                        mes.comm, &requests[i]);
                assert(MPI_SUCCESS == ret_val);
                MPIPCL_DEBUG(
                    "Recv_init called - buffer: %p - req pointers: %p\n",
                    (void*)(void*)((char*)mes.buff + offset),
                    (void*)&requests[i]);
            }

            // Since they're atomic, might as well do the atomic init call
            std::atomic_init(&internal_status[i], 0);
            complete[i] = 0;
        }
    }

    void threaded_sync_driver()
    {
        if (side == P2PSide::RECEIVER)
        {
            sync_side(P2PSide::SENDER);
        }
        else
        {
            sync_side(P2PSide::RECEIVER);
        }

        internal_setup();

        // lock and check request status.
        pthread_mutex_lock(&lock);
        // if request is active(MPI_Start), run catchup tasks.
        if (side == P2PSide::RECEIVER && state == Activation::ACTIVE)
        {
            MPIPCL_DEBUG("%d THREAD IS STARTING RECVS:%d \n", side, size);
            int ret_val = MPI_Startall(parts, requests.data());
            assert(MPI_SUCCESS == ret_val);
        }
        else if (side == P2PSide::SENDER && state == Activation::ACTIVE)
        {
            MPIPCL_DEBUG("%d THREAD IS STARTING SENDS:%d \n", side, size)
            send_ready();
        }
        // once caught up, signal thread completion and return.
        threaded = FINISHED;
        pthread_mutex_unlock(&lock);
    }

    void send_ready()
    {
        // call send on each currently marked partition
        for (int i = 0; i < local_parts; i++)
        {
            if (local_status[i])
            {
                general_send(i);
            }
        }
    }

    void general_send(int id)
    {
        int start_part = 0;
        int end_part   = 0;
        int threshold  = 0;
        MPIPCL_DEBUG("User Partitions %d - Network Partitions %d\n",
                     local_parts, parts);
        if (local_parts <= parts)
        {
            map_local_to_network_partitions(id, &start_part, &end_part);
            threshold = 1;
        }
        else /* Must be: if(local_parts > parts) */
        {
            threshold  = local_parts / parts;
            start_part = id / threshold;
            end_part   = start_part + 1;
        }

        MPIPCL_DEBUG("User Partition %d - Start %d End %d \n", id, start_part,
                     end_part);

        // for each internal request affected
        for (int i = start_part; i < end_part; i++)
        {
            // increase the number of "local" partitions ready (using atomics)
            int prior_value = std::atomic_fetch_add_explicit(
                &internal_status[i], 1, std::memory_order_relaxed);

            MPIPCL_DEBUG("Network Partition %d - Count: %d, Threshold: %d\n", i,
                         prior_value + 1, threshold);

            // if the number of local partitions needed for one network
            // partition are ready
            if (prior_value + 1 == threshold)
            {
                // start associated request
                MPIPCL_DEBUG("Starting request %d %p \n", i,
                             (void*)(&requests[i]));
                int ret_val = MPI_Start(&requests[i]);
                assert(MPI_SUCCESS == ret_val);
            }
        }
    }

    inline void map_local_to_network_partitions(int  user_partition_id,
                                                int* start, int* end)
    {
        int user_partitions    = local_parts;
        int network_partitions = parts;

        int temp_start =
            (network_partitions / user_partitions) * user_partition_id;
        int temp_end = ceil(network_partitions / user_partitions) *
                       (user_partition_id + 1);

        MPIPCL_DEBUG("Partition %d - Start %d End %d \n", user_partition_id,
                     temp_start, temp_end);
        assert(temp_start >= 0 && temp_end > temp_start &&
               temp_end <= network_partitions);

        *start = temp_start;
        *end   = temp_end;
    }

    // maps recv_buffer offset -- parried related.
    // returns 1 if ready, 0 otherwise.
    // all results from race condition identical, no lock needed.
    inline bool map_recv_buffer(int id)
    {
        if (local_status[id] == true)
        {
            return 1;
        }

        int start = 0;
        int end   = 0;
        if (local_parts <= parts)
        {
            map_local_to_network_partitions(id, &start, &end);
        }
        else
        {
            int threshold = local_parts / parts;
            start         = id / threshold;
            end           = start + 1;
        }

        // check status of dependent requests
        int flag = 0;
        MPIPCL_DEBUG("User Partitions %d - Network Partitions %d\n",
                     local_parts, parts);
        MPIPCL_DEBUG("Checking Requests: [%d: %d)\n", start, end);
        int ret_val = MPI_Testall(end - start, &requests[start], &flag,
                                  MPI_STATUSES_IGNORE);
        assert(MPI_SUCCESS == ret_val);

        // if true store for future shortcut
        local_status[id] = flag;

        return flag;
    }

    enum Activation state;
    enum P2PSide    side;
    std::vector<bool> local_status;  // status array - true if external partition is
                         // ready
    std::vector<std::atomic<int>> internal_status;
    std::vector<bool> complete;  // status array - true if internal request has
                                 // been started.

    int local_parts;  // number of partitions visible externally
    int local_size;   // number of items in each partitions

    int parts;  // number of internal requests to complete
    int size;   // number of items in each internal request
    std::vector<MPI_Request>
        requests;  // array of "size" internal requests to process

    // Struct of message data to setup internal requests away from Init
    // function
    MessageData comm_data;

    // thread variables to enable background sync if necessary
    std::thread     sync_thread;
    pthread_mutex_t lock;
    ThreadStatus    threaded;  // status of sync thread
                               // "-1"-no_thread, 0-exist, 1-finished
};
}  // namespace mpipcl
}  // namespace MPIAdvance
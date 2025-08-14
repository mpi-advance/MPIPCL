/* C++ version of Test 8 to make sure compiling
 * and linking works correctly.
 *
 * To run:
 *    mpirun -np 2 ./<test>
 */

#include <string>

#include "mpi.h"
#include "mpipcl.h"

static inline void exchange(
    int rank, double* buf, int bufsize, int nparts, MPI_Info the_info)
{
    int i, j;
    int tag = 0xbad;
    MPIP_Request req;
    MPI_Status status;

    int count = bufsize / nparts;
    if (0 == rank)
    { /* sender */
        MPIP_Psend_init(
            buf, nparts, count, MPI_DOUBLE, 1, tag, MPI_COMM_WORLD, the_info, &req);
        MPIP_Start(&req);

#pragma omp parallel for private(j) shared(buf, req) num_threads(nparts)
        for (i = 0; i < nparts; i++)
        {
            /* initialize part of buffer in each thread */
            for (j = 0; j < count; j++)
                buf[j + i * count] = j + i * count + 1.0;

            /* indicate buffer is ready */
            MPIP_Pready(i, &req);
        }

        MPIP_Wait(&req, &status);
    }
    else
    { /* receiver */
        MPIP_Precv_init(
            buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, the_info, &req);
        MPIP_Start(&req);

        for (i = 0; i < nparts; i++)
        {
            int arrived = 0;
            while (0 == arrived)
            {
                MPIP_Parrived(&req, i, &arrived);
            }
            printf("Done with partition %d\n", i);
        }

        double sum = 0.0;
        /* compute the sum of the values received */
        for (i = 0, sum = 0.0; i < bufsize; i++)
            sum += buf[i];

        MPIP_Wait(&req, &status);
        printf("#partitions = %d bufsize = %d count = %d sum = %f\n",
               nparts,
               bufsize,
               count,
               sum);
    }
    MPIP_Request_free(&req);
}

int main(int argc, char* argv[])
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    assert(provided == MPI_THREAD_SERIALIZED);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if ((size != 2))
    {
        printf("comm size must be 2 \n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    int string_size         = 20;
    std::string HARD_NUMBER = "4";
    int nparts              = atoi(HARD_NUMBER.c_str());

    int bufsize = nparts * (nparts / 2) * (nparts * 2);
    double* buf = new double[bufsize];

    MPI_Info the_info;
    MPI_Info_create(&the_info);
    MPI_Info_set(the_info, "PMODE", "HARD");
    MPI_Info_set(the_info, "SET", HARD_NUMBER.c_str());

    if (1 == rank)
    {
        printf("Testing partitions equal to \"HARD\" preset: %d\n", nparts);
    }
    exchange(rank, buf, bufsize, nparts, the_info);

    nparts = nparts * 2;
    if (1 == rank)
    {
        printf("Testing partitions equal to 2x \"HARD\" preset: %d\n", nparts);
    }
    exchange(rank, buf, bufsize, nparts, the_info);

    nparts = nparts / 4;
    if (1 == rank)
    {
        printf("Testing partitions equal to 1/2 \"HARD\" preset: %d\n", nparts);
    }
    exchange(rank, buf, bufsize, nparts, the_info);

    MPI_Info_free(&the_info);

    delete[] buf;
    MPI_Finalize();

    return 0;
}

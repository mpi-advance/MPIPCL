/* Sample program to test the partitioned communication API.
 * This version uses threads on the send-side only, but allows
 * users to change the behind the scenes mode of partition
 * negotiation.
 *
 * To run:
 *    mpirun -np 2 ./<test> <npartitions> <bufsize> <mode>
 *    WHERE <mode> = 0 MPI_INFO_NULL -- library default)
 *                   1 PMODE = SENDER -- sender's partitions
 *                   2 PMODE = RECEIVER -- receiver's partitions
 *                   3 PMODE = HARD  -- a specific number chosen,
 *                              controlled by "HARD_NUMBER" below
 *    NOTE: bufsize % npartitions == 0
 */
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpipcl.h"

const char HARD_NUMBER = '2';

int main(int argc, char* argv[])
{
    int rank, size, nparts, mode, bufsize, count, tag = 0xbad;
    int i, j, provided;
    double *buf, sum;
    MPIX_Request req;
    MPI_Status status;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    assert(provided == MPI_THREAD_SERIALIZED);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    assert(size == 2);

    if (argc != 4)
    {
        printf("Usage: %s <#partitions> <bufsize> <mode>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    nparts  = atoi(argv[1]);
    bufsize = atoi(argv[2]);
    count   = bufsize / nparts;
    mode    = atoi(argv[3]);
    if (mode < 0 || mode > 3)
    {
        printf("Invalid mode for this test.\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    MPI_Info the_info;
    if (mode == 0)
    {
        the_info = MPI_INFO_NULL;
    }
    else if (mode == 1)
    {
        MPI_Info_create(&the_info);
        MPI_Info_set(the_info, "PMODE", "SENDER");
    }
    else if (mode == 2)
    {
        MPI_Info_create(&the_info);
        MPI_Info_set(the_info, "PMODE", "RECEIVER");
    }
    else
    {
        MPI_Info_create(&the_info);
        MPI_Info_set(the_info, "PMODE", "HARD");
        MPI_Info_set(the_info, "SET", &HARD_NUMBER);
    }

    if ((size != 2) || (bufsize % nparts != 0))
    {
        printf("comm size must be 2 and bufsize must be divisible by nparts\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    buf = malloc(sizeof(double) * bufsize);

    if (rank == 0)
    { /* sender */
        MPIX_Psend_init(buf, nparts, count, MPI_DOUBLE, 1, tag, MPI_COMM_WORLD,
                        the_info, &req);
        MPIX_Start(&req);

#pragma omp parallel for private(j) shared(buf, req) num_threads(nparts)
        for (i = 0; i < nparts; i++)
        {
            /* initialize part of buffer in each thread */
            for (j = 0; j < count; j++)
                buf[j + i * count] = j + i * count + 1.0;

            /* indicate buffer is ready */
            MPIX_Pready(i, &req);
        }

        MPIX_Wait(&req, &status);
    }
    else
    { /* receiver */
        MPIX_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD,
                        the_info, &req);
        MPIX_Start(&req);
        MPIX_Wait(&req, &status);

        /* compute the sum of the values received */
        for (i = 0, sum = 0.0; i < bufsize; i++)
            sum += buf[i];

        printf("#partitions = %d bufsize = %d count = %d sum = %f\n", nparts,
               bufsize, count, sum);
    }

    if (mode != 0)
        MPI_Info_free(&the_info);
    MPIX_Request_free(&req);
    free(buf);
    MPI_Finalize();

    return 0;
}

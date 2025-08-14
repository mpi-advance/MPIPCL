/* Sample program to test the partitioned communication API,
 * specifically the use of parrived "to make progress." This
 * program is also testing out that the mapping of "user
 * partitions" can correctly be mapped to "network partitions"
 * by making the receive side request double the number of partitions
 * as the send side needs.
 *
 * To run:
 *    mpirun -np 2 ./<exec> <npartitions> <bufsize>
 *    NOTE: bufsize % npartitions == 0 and
 *          bufsize % (npartitions * 2) == 0
 */
#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpipcl.h"

int main(int argc, char* argv[])
{
    int rank, size, nparts, bufsize, count, tag = 0xbad;
    int i, j, provided;
    double *buf, sum;
    MPIP_Request req;
    MPI_Status status;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    assert(provided == MPI_THREAD_SERIALIZED);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    assert(size == 2);

    if (argc != 3)
    {
        printf("Usage: %s <#partitions> <bufsize>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    nparts  = atoi(argv[1]);
    bufsize = atoi(argv[2]);
    count   = bufsize / nparts;

    if ((size != 2) || (bufsize % nparts != 0))
    {
        printf("comm size must be 2 and bufsize must be divisible by nparts\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if (bufsize % (2 * nparts) != 0)
    {
        printf("bufsize must also be divisible by twice nparts\n");
        MPI_Abort(MPI_COMM_WORLD, -2);
    }

    buf = malloc(sizeof(double) * bufsize);

    if (rank == 0)
    { /* sender */
        MPIP_Psend_init(
            buf, nparts, count, MPI_DOUBLE, 1, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req);
        MPIP_Start(&req);

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
        nparts *= 2;
        count = count / 2;
        MPIP_Precv_init(
            buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req);
        MPIP_Start(&req);

        for (i = 0; i < nparts; i++)
        {
            int done = 0;
            while (!done)
            {
                MPIP_Parrived(&req, i, &done);
            }
            printf("Done with partition %d\n", i);
        }

        MPIP_Wait(&req, &status);

        /* compute the sum of the values received */
        for (i = 0, sum = 0.0; i < bufsize; i++)
            sum += buf[i];

        printf("#partitions = %d bufsize = %d count = %d sum = %f\n",
               nparts,
               bufsize,
               count,
               sum);
    }

    MPIP_Request_free(&req);
    free(buf);
    MPI_Finalize();

    return 0;
}

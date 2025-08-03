/* Sample program to test the partitioned communication API.
 * This version uses threads on the send-side only.
 *
 * To compile:
 *    mpicc -O -Wall -fopenmp -o mpipcltest1 mpipcltest1.c mpipcl.c
 * To run:
 *    mpirun -np 2 ./mpipcltest1 <npartitions> <bufsize>
 *    NOTE: bufsize % npartitions == 0
 */
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpipcl.h"

int main(int argc, char* argv[])
{
    int rank, size, nparts, bufsize, count, tag = 0xbad;
    int i, j, provided;
    double *buf, sum;
    MPIA_Request req;
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

    buf = malloc(sizeof(double) * bufsize);

    if (rank == 0)
    { /* sender */
        MPIA_Psend_init(buf, nparts, count, MPI_DOUBLE, 1, tag, MPI_COMM_WORLD,
                        MPI_INFO_NULL, &req);
        MPIA_Start(&req);

#pragma omp parallel for private(j) shared(buf, req) num_threads(nparts)
        for (i = 0; i < nparts; i++)
        {
            /* initialize part of buffer in each thread */
            for (j = 0; j < count; j++)
                buf[j + i * count] = j + i * count + 1.0;

            /* indicate buffer is ready */
            MPIA_Pready(i, &req);
        }

        MPIA_Wait(&req, &status);
    }
    else
    { /* receiver */
        MPIA_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD,
                        MPI_INFO_NULL, &req);
        MPIA_Start(&req);
        MPIA_Wait(&req, &status);

        /* compute the sum of the values received */
        for (i = 0, sum = 0.0; i < bufsize; i++)
            sum += buf[i];

        printf("#partitions = %d bufsize = %d count = %d sum = %f\n", nparts,
               bufsize, count, sum);
    }

    MPIA_Request_free(&req);
    free(buf);
    MPI_Finalize();

    return 0;
}

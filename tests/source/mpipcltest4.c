/* Sample program to test the partitioned communication API.
 * This program tests MPIP_Startall, MPIP_Pready, and MPIP_Waitall functions.
 * This version uses threads on both send-side and receive-side.
 *
 * To compile:
 *    mpicc -O -Wall -fopenmp -o mpipcltest4 mpipcltest4.c mpipcl.c
 * To run:
 *    mpirun -np 5 ./mpipcltest4 <npartitions> <bufsize>
 *    NOTE: bufsize % npartitions == 0
 */
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpipcl.h"

#define NNEIGHBORS 4

int main(int argc, char* argv[])
{
    int rank, size, nparts, bufsize, count, tag = 0xbad, rc = 0;
    int i, j, provided;
    double *buf, sum;
    MPIP_Request req[NNEIGHBORS];

    MPI_Init_thread(&argc, &argv, MPI_THREAD_SERIALIZED, &provided);
    assert(provided == MPI_THREAD_SERIALIZED);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 3)
    {
        printf("Usage: %s <#partitions> <bufsize>\n", argv[0]);
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    nparts  = atoi(argv[1]);
    bufsize = atoi(argv[2]);
    count   = bufsize / nparts;

    if ((size != 5) || (bufsize % nparts != 0))
    {
        printf("comm size must be 5 and bufsize must be divisible by nparts\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }
    buf = malloc(sizeof(double) * bufsize);
    assert(buf != NULL);

    if (rank == 0)
    { /* sender */
        for (i = 1; i <= NNEIGHBORS; i++)
        {
            rc = MPIP_Psend_init(buf,
                                 nparts,
                                 count,
                                 MPI_DOUBLE,
                                 i,
                                 tag,
                                 MPI_COMM_WORLD,
                                 MPI_INFO_NULL,
                                 &req[i - 1]);
            assert(rc == MPI_SUCCESS);
        }

        rc = MPIP_Startall(NNEIGHBORS, req);
        assert(rc == MPI_SUCCESS);
        printf("[%d]: nparts = %d bufsize = %d count = %d size = %d\n",
               rank,
               nparts,
               bufsize,
               count,
               size);

#pragma omp parallel for private(j) shared(buf, req) num_threads(nparts)
        for (i = 0; i < nparts; i++)
        {
            /* initialize part of buffer in each thread */
            for (j = 0; j < count; j++)
            {
                buf[j + i * count] = j + i * count + 1.0;
            }

            /* indicate buffer is ready for all sends */
            for (j = 0; j < NNEIGHBORS; j++)
            {
                rc = MPIP_Pready(i, &req[j]);
                assert(rc == MPI_SUCCESS);
            }
        }

        rc = MPIP_Waitall(NNEIGHBORS, req, MPI_STATUSES_IGNORE);
        assert(rc == MPI_SUCCESS);
        for (i = 0; i < NNEIGHBORS; i++)
        {
            rc = MPIP_Request_free(&req[i]);
            assert(rc == MPI_SUCCESS);
        }
    }
    else
    { /* receiver */
        MPIP_Request req;

        rc = MPIP_Precv_init(
            buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, MPI_INFO_NULL, &req);
        assert(rc == MPI_SUCCESS);
        rc = MPIP_Start(&req);
        assert(rc == MPI_SUCCESS);

#pragma omp parallel for shared(buf, req, sum) num_threads(nparts)
        for (i = 0; i < nparts; i++)
        {
            int j, flag = 0, testflag;
            double mysum = 0.0;
            while (!flag)
            {
                /* check if partition has been received */
                rc = MPIP_Parrived(&req, i, &flag);
                assert(rc == MPI_SUCCESS);

                if (flag)
                {
                    /* compute the partial sum of the values received */
                    for (j = 0, mysum = 0.0; j < count; j++)
                    {
                        mysum += buf[j + i * count];
                    }

/* update global sum */
#pragma omp critical
                    sum += mysum;
                }
                else
                {
                    /* do some other work */
                    rc = MPIP_Test(&req, &testflag, MPI_STATUS_IGNORE);
                    assert(rc == MPI_SUCCESS);
                    /* do some other work based on testflag */
                }
            }
        }
        rc = MPIP_Wait(&req, MPI_STATUS_IGNORE);
        assert(rc == MPI_SUCCESS);
        printf("[%d]: #partitions = %d bufsize = %d count = %d sum = %f (%f)\n",
               rank,
               nparts,
               bufsize,
               count,
               sum,
               ((double)bufsize * (bufsize + 1)) / 2.0);
        rc = MPIP_Request_free(&req);
        assert(rc == MPI_SUCCESS);
    }

    free(buf);
    MPI_Finalize();

    return 0;
}

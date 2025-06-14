/* Sample program to test the partitioned communication API.

 * This program tests MPIX_Waitsome and MPIX_Testsome
 * MPIX_Waitall functions.
 *
 * To compile:
 *    mpicc -O -Wall -fopenmp -o mpipcltest5 mpipcltest5.c mpipcl.c
 * To run:
 *    mpirun -np 2 ./mpipcltest5 <npartitions> <bufsize>

 *    NOTE: bufsize % npartitions == 0
 */
#include <assert.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpipcl.h"

int main(int argc, char* argv[])
{
    const int NUMREQ = 2;
    int rank, size, nparts, bufsize, count, rc = 0;
    int provided;
    double* buf;
    MPIX_Request req[NUMREQ];

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

    if ((size != 2) || (bufsize % nparts != 0))
    {
        printf("comm size must be 2 and bufsize must be divisible by nparts\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    buf = malloc(sizeof(double) * bufsize);
    assert(buf != NULL);

    /* init buffers */
    for (int i = 0; i < bufsize; i++)
    {
        if (rank != 0)
            buf[i] = 0.0;
        else
            buf[i] = i + 1.0;
    }
    printf("buffer: %p\n", (void*)buf);

    if (rank == 0)
    { /* sender */
        /* make the requests */
        for (int i = 0; i < NUMREQ; i++)
        {
            rc = MPIX_Psend_init(buf, nparts, count, MPI_DOUBLE, 1, i,
                                 MPI_COMM_WORLD, MPI_INFO_NULL, &req[i]);
            assert(rc == MPI_SUCCESS);
        }

        for (int j = 0; j < NUMREQ; j++)
        {
            /* start request */
            rc = MPIX_Start(&req[j]);
            assert(rc == MPI_SUCCESS);

            /* indicate buffer is ready */
            rc = MPIX_Pready_range(0, nparts - 1, &req[j]);
            assert(rc == MPI_SUCCESS);

            /* wait for first request to complete before starting next */
            rc = MPIX_Wait(&req[j], MPI_STATUSES_IGNORE);
            assert(rc == MPI_SUCCESS);

            /* clean up */
            rc = MPIX_Request_free(&req[j]);
            assert(rc == MPI_SUCCESS);
        }
    }
    else if (rank == 1)
    { /* receiver */

        /* make requests */
        for (int i = 0; i < NUMREQ; i++)
        {
            rc = MPIX_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, i,
                                 MPI_COMM_WORLD, MPI_INFO_NULL, &req[i]);
            assert(rc == MPI_SUCCESS);
        }

        /* start all but last request */
        for (int k = 0; k < NUMREQ - 1; k++)
        {
            rc = MPIX_Start(&req[k]);
            assert(rc == MPI_SUCCESS);
        }

        /* wait for a request to complete */
        int indices[NUMREQ];
        int complete = -1;  // number of complete requests
        rc = MPIX_Waitsome(NUMREQ, req, &complete, indices, MPI_STATUS_IGNORE);
        assert(rc == MPI_SUCCESS);

        printf("Wait completed: %d (count: %d) \n", indices[0], complete);
        for (int h = 0; h < complete; h++)
        {
            printf("%d ", indices[h]);
            indices[h] = -1;
        }
        printf("\n");

        /* Testsome -- see if incomplete request is returned (it shouldn't be)
         */
        complete = -1;
        rc = MPIX_Testsome(NUMREQ, req, &complete, indices, MPI_STATUS_IGNORE);
        assert(rc == MPI_SUCCESS);

        printf("Testsome complete count: %d \n", complete);
        for (int i = 0; i < complete; i++)
            printf("%d ", indices[i]);
        printf("\n");

        /* start final request */
        rc = MPIX_Start(&req[NUMREQ - 1]);
        assert(rc == MPI_SUCCESS);

        /* wait for second to complete */
        rc = MPIX_Waitall(NUMREQ, req, MPI_STATUS_IGNORE);
        assert(rc == MPI_SUCCESS);
        printf("Second request complete\n");

        /* compute the sum of the values received */
        double sum = 0.0;
        for (int i = 0; i < bufsize; i++)
            sum += buf[i];

        for (int j = 0; j < NUMREQ; j++)
        {
            rc = MPIX_Request_free(&req[j]);
            assert(rc == MPI_SUCCESS);
        }

        printf("[%d]: #partitions = %d bufsize = %d count = %d sum = %f (%f)\n",
               rank, nparts, bufsize, count, sum,
               ((double)bufsize * (bufsize + 1)) / 2.0);
    }

    free(buf);
    MPI_Finalize();

    return rc;
}

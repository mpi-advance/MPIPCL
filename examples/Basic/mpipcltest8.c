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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <omp.h>
#include "mpipcl.h"

static inline void exchange(int rank, int nparts, MPI_Info the_info)
{
    int i, j;
    int tag = 0xbad;
    MPIX_Request req;
    MPI_Status status;

    int bufsize = nparts*(nparts/2)*(nparts*2);
    int count = bufsize / nparts;
    double* buf = malloc(sizeof(double) * bufsize);

    if (0 == rank)
    { /* sender */
        MPIX_Psend_init(buf, nparts, count, MPI_DOUBLE, 1, tag, MPI_COMM_WORLD, the_info, &req);
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
        MPIX_Precv_init(buf, nparts, count, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, the_info, &req);
        MPIX_Start(&req);
    
        for(i = 0; i < nparts; i++)
        {
            int arrived = 0;
            while(0 == arrived)
            {
                MPIX_Parrived(&req, i, &arrived);
            }
            printf("Done with partition %d\n", i);
        }
    
        double sum = 0.0;
        /* compute the sum of the values received */
        for (i = 0, sum = 0.0; i < bufsize; i++)
            sum += buf[i];
        
        MPIX_Wait(&req, &status);
        printf("#partitions = %d bufsize = %d count = %d sum = %f\n",
               nparts, bufsize, count, sum);
    }
    MPIX_Request_free(&req);
    free(buf);
}

int main(int argc, char *argv[])
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

    int string_size = 20;
    char* HARD_NUMBER = malloc(sizeof(char)*string_size);
    HARD_NUMBER[0] = '4';
    HARD_NUMBER[1] = '\0';
    int nparts = atoi(HARD_NUMBER);

    MPI_Info the_info;
    MPI_Info_create(&the_info);
    MPI_Info_set(the_info, "PMODE", "HARD");
    MPI_Info_set(the_info, "SET", HARD_NUMBER);

    if(1 == rank)
    {
        printf("Testing partitions equal to \"HARD\" preset: %d\n", nparts);
    }
    exchange(rank, nparts, the_info);

    nparts = nparts*2;
    if(1 == rank)
    {
        printf("Testing partitions equal to 2x \"HARD\" preset: %d\n", nparts);
    }
    exchange(rank, nparts, the_info);

    nparts = nparts/4;
    if(1 == rank)
    {
        printf("Testing partitions equal to 1/2 \"HARD\" preset: %d\n", nparts);
    }
    exchange(rank, nparts, the_info);

    MPI_Info_free(&the_info);

    free(HARD_NUMBER);
    MPI_Finalize();

    return 0;
}

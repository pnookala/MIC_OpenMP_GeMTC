/*
 * Sleep.c
 *
 *  Created on: Apr 22, 2015
 *      Author: pnookala
 */
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "Sleep.h"
#include <float.h>
#include "omp.h"
#include <sys/time.h>

void *GetResponse();

struct timeval startTime, endTime, timeDifference;
int s2;

void Sleep(void *params, int num_threads)
{
	omp_set_num_threads(num_threads);

	printf("Starting Sleep Jobs\n");

	int i;

	int *sleepTime = (int *)params;
	printf("Offloading to MIC... \n");

gettimeofday(&startTime, NULL);

#pragma offload target(mic:MIC_DEV) \
	 in(*sleepTime:length(sizeof(int))) signal(s2)
{
                int dummy = 1;
                int threadId = omp_get_thread_num();
        //      printf("ThreadID: %d \n", threadId);

                struct timeval tvBegin, tvEnd, tvDiff;

                // begin
                gettimeofday(&tvBegin, NULL);

                //printf("Start Time: ", tvBegin.tv_usec);

                gettimeofday(&tvEnd, NULL);
                //printf("Start Time: ", tvBegin.tv_usec);
                if (*sleepTime) {

                	while( (tvEnd.tv_usec - tvBegin.tv_usec) <  *sleepTime) {
                                int i, j;
                                for (i = 0; i < 10000; ++i) {
                                        dummy *= 2;
                                }

                                gettimeofday(&tvEnd, NULL);
                                //printf("Time Elapsed: %ld seconds, ThreadID: %d \n",((tvEnd.tv_usec - tvBegin.tv_usec) / 1000000 + (tvEnd.tv_sec - tvBegin.tv_sec)), threadId);
                             	// printf( "Thread Num : %d, Sleep Duration (usec): %d, Execution Time: %ld, Theoritical Time: %ld\n", threadId, sleepTime, (tvEnd.tv_usec - tvBegin.tv_usec), sleepMicroSeconds);
				// printf("Time Elapsed: %ld microseconds, ThreadID: %d \n",(tvEnd.tv_usec - tvBegin.tv_usec), threadId);

			}
	}
}
//#pragma offload_wait target(mic:MIC_DEV) wait(s2)
//gettimeofday(&endTime, NULL);
	pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
	pthread_create(bg, NULL, GetResponse, NULL);

//		printf( "Sleep Duration (usec): %d, Execution Time: %ld\n", *sleepTime, (endTime.tv_usec - startTime.tv_usec));
                

                //printf("End Time: ", tvEnd.tv_sec);
	

}

void *GetResponse()
{
	#pragma offload_wait target(mic:MIC_DEV) wait(s2)
	gettimeofday(&endTime, NULL);

	printf( "Execution Time: %ld\n", (endTime.tv_usec - startTime.tv_usec));
}

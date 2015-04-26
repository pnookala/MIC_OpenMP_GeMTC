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
#include <pthread.h>

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

		struct timeval tvBegin, tvEnd, tvDiff;

		// begin
		gettimeofday(&tvBegin, NULL);

		gettimeofday(&tvEnd, NULL);

		if (*sleepTime) {

			while( (tvEnd.tv_usec - tvBegin.tv_usec) <  *sleepTime) {
				int i, j;
				for (i = 0; i < 10000; ++i) {
					dummy *= 2;
				}

				gettimeofday(&tvEnd, NULL);

			}
		}
	}
	//Spawn a new thread to wait for the results from Xeon Phi
	pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
	pthread_create(bg, NULL, GetResponse, NULL);

}

void *GetResponse()
{
#pragma offload_wait target(mic:MIC_DEV) wait(s2)
	gettimeofday(&endTime, NULL);

	printf( "Execution Time: %ld\n", (endTime.tv_usec - startTime.tv_usec));
}

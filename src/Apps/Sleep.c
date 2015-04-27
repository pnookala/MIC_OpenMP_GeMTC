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
#include <unistd.h>

void *GetResponse();

struct timeval startTime, endTime, timeDifference;
int s2;

void Sleep(void *params, int num_threads)
{
	omp_set_num_threads(num_threads);

	int i;

	int *sleepTime = (int *)params;
	printf("Starting Sleep Job for %d seconds \n", *sleepTime);
	printf("Offloading to MIC... \n");

	gettimeofday(&startTime, NULL);

#pragma offload target(mic:MIC_DEV) \
		in(*sleepTime:length(sizeof(int))) signal(s2)
	{
	/*	int dummy = 1;

		struct timeval tvBegin, tvEnd;
		double tvDiff;

		// begin
		gettimeofday(&tvBegin, NULL);

		gettimeofday(&tvEnd, NULL);

		if (*sleepTime) {
			tvDiff = ((tvEnd.tv_usec - tvBegin.tv_usec)/1000000 + (tvEnd.tv_sec - tvBegin.tv_sec));
			while( tvDiff  <  *sleepTime) {
				int i, j;
				for (i = 0; i < 10000; ++i) {
					dummy *= 2;
				}

				gettimeofday(&tvEnd, NULL);

			}
		}*/
		sleep(*sleepTime);
	}
	printf("Offloaded async computation\n");
	//Spawn a new thread to wait for the results from Xeon Phi
	pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
	pthread_create(bg, NULL, GetResponse, NULL);

}

void *GetResponse()
{
	printf("Waiting for signal\n");
#pragma offload_wait target(mic:MIC_DEV) wait(s2)
	printf("Signal received\n");
	gettimeofday(&endTime, NULL);
	printf("End Time in seconds: %ld.%06ld\n", (long int)endTime.tv_sec, (long int)endTime.tv_usec);
	printf("Start Time in seconds: %ld.%06ld\n", (long int)startTime.tv_sec, (long int)startTime.tv_usec);
	printf("Execution Time in seconds: %ld.%06ld\n", (long int)(endTime.tv_sec-startTime.tv_sec), (long int)(endTime.tv_usec-startTime.tv_usec));
//	printf( "Start Time: %f, End Time: %f, Execution Time: %f\n", startTime.tv_usec, endTime.tv_usec, ((endTime.tv_usec - startTime.tv_usec) / 1000000 + (endTime.tv_sec - startTime.tv_sec)));
}

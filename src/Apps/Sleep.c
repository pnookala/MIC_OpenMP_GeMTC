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

void *GetResponse(void *data);
__attribute__ ((target(mic))) void DoSleep(void *par);

struct timeval startTime, endTime, timeDifference;

void Sleep(void *params, int num_threads, int task_id)
{
	omp_set_num_threads(num_threads);

	printf("Starting Sleep Job\n");

	int i;
	
	int *sleepTime = (int *)params;
	printf("Offloading to MIC... \n");

	gettimeofday(&startTime, NULL);
	int s2 = task_id;
#pragma offload target(mic:MIC_DEV) \
		in(*sleepTime:length(sizeof(int))) signal(s2)
#pragma omp single
	{
		int dummy = 1;

		struct timeval tvBegin, tvEnd, tvDiff;

		// begin
		gettimeofday(&tvBegin, NULL);
//		printf("%ld \n", tvBegin.tv_usec);
		gettimeofday(&tvEnd, NULL);
//		printf("%ld \n", tvEnd.tv_usec);
		if (*sleepTime) {
//printf("Sleep Time: %ld\n", *sleepTime);
	while( (((tvEnd.tv_usec - tvBegin.tv_usec))+(tvEnd.tv_sec - tvBegin.tv_sec)*1000000) <  *sleepTime) {
				int i, j;
				for (i = 0; i < 10000; ++i) {
					dummy *= 2;
				}

				gettimeofday(&tvEnd, NULL);
//				printf("%ld\n", (((tvEnd.tv_usec - tvBegin.tv_usec))+(tvEnd.tv_sec - tvBegin.tv_sec)*1000000));
			}
		}
	//	sleep(*sleepTime);
	}
	printf("sleep task completed...");

	//Spawn a new thread to wait for the results from Xeon Phi
	int *localCopy = (int*)malloc(sizeof(int));
	*localCopy = s2;
	//printf("Local copy: %d\n",*localCopy);
	pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
	pthread_create(bg, NULL, GetResponse, (void *)localCopy);

}

void *GetResponse(void *data)
{

	printf("Waiting for signal\n");
	int *v = (int *)data;
	int value = *v;
	printf("Value v: %d\n", value);
#pragma offload_wait target(mic:MIC_DEV) wait(value)
	printf("Signal received %d\n", value);
	free(v);
	gettimeofday(&endTime, NULL);
	printf("End Time in seconds: %ld.%06ld\n",(long int)endTime.tv_sec, (long int)endTime.tv_usec);
	printf("Start Time in seconds: %ld.%06ld\n", (long int)startTime.tv_sec, (long int)startTime.tv_usec);
	printf("Execution Time in seconds: %ld.%06ld\n", (long int)(endTime.tv_sec-startTime.tv_sec), (long int)(endTime.tv_usec-startTime.tv_usec));
//	printf( "Start Time: %f, End Time: %f, Execution Time: %f\n", startTime.tv_usec, endTime.tv_usec, ((endTime.tv_usec - startTime.tv_usec) / 1000000 + (endTime.tv_sec - startTime.tv_sec)));
}

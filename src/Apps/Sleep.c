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

struct timeval startTime, endTime, timeDifference;

void Sleep(void *params, int num_threads, int task_id)
{
	#pragma omp single
	{
		printf("Starting Sleep Job\n");
	}

	int *sleepTime = (int *)params;
	sleep(*sleepTime);
}

/*
 * MasterProcess.c
 *
 *  Created on: Apr 25, 2015
 *      Author: pnookala, kstough
 *  Main scheduler/handler for offloading jobs to MIC
 */

#include "MasterProcess.h"

#ifndef MIC_DEV
#define MIC_DEV 0
#endif

// Also see Makefile, options can be specified there as well with "-D<Option>
// To offload work to MIC
//#ifndef OFFLOAD
//#define OFFLOAD
//#endif

// Enable this to force only one task to execute at a time
//#ifndef SINGLE_TASK
//#define SINGLE_TASK
//#endif

// To run host scheduler in parallel (doesn't work yet)
//#ifndef HOST_PARALLEL
//#define HOST_PARALLEL
//#endif

// main callback for completed tasks
void *GetResponse(void *data);

#ifdef OFFLOAD
__attribute__ ((target(mic))) void Sleep(void *params, int num_threads, int task_id);
__attribute__ ((target(mic))) void MatrixMultiplication(int sqrtElements, int numThreads);
#else
void Sleep(void *params, int num_threads, int task_id);
void MatrixMultiplication(int sqrtElements, int numThreads);
#endif

// main task scheduling section, add other methods in switch statement here
struct task_desc *execute_task(struct task_desc *task)
{
	//printf("In execute task...\n");
	struct timeval sTime;
	gettimeofday(&sTime, NULL);
	double startSeconds = sTime.tv_sec + ((double)sTime.tv_usec)/1000000.0;
	task->startTime = startSeconds;
	
	int* parameters = task->params;
	int task_id = task->task_id;
	int task_type = task->task_type;
	int num_threads = task->num_threads;
	
#ifdef OFFLOAD
#ifdef SINGLE_TASK
	#pragma offload target(mic:MIC_DEV) \
		in(*parameters:length(sizeof(int))) \
		in(task_id, task_type, num_threads)
#else
	#pragma offload target(mic:MIC_DEV) \
		in(*parameters:length(sizeof(int))) \
		in(task_id, task_type, num_threads) \
		signal(&(task->signal))
#endif
#endif
	{
		// #pragma omp _something_? Need to do a better job of 
		// getting multiple tasks to run concurrently on the Phi
		{
		switch (task_type) {
			case 1:
				Sleep((int *)parameters, num_threads, task_id);
				break;
				
			case 2:
			{
				int *size = (int *)parameters;
				MatrixMultiplication(*size, num_threads);
				break;
			}
			default:
				//printf("Not supported: %d\n", task_type);
				break;
		}
		}
	}
	
#ifdef SINGLE_TASK
	GetResponse((void *)task);
#else
	// critical section may not be needed? Needs testing
	#pragma omp critical
	{
		struct task_desc *localCopy = malloc(sizeof(struct task_desc));
		localCopy = task;
		pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
		pthread_create(&bg, NULL, GetResponse, (void *)localCopy);
	}
#endif

	//printf("MasterProcess, execute_task, finish\n");
	return task;
}

void *GetResponse(void *data)
{
	//printf("Waiting for signal\n");
	
	struct task_desc *v = (struct task_desc *)data;
	int taskId = v->task_id;
	double startSeconds = v->startTime;
	
#ifndef SINGLE_TASK
#ifdef OFFLOAD
	#pragma offload_wait target(mic:MIC_DEV) wait(&(v->signal))
#endif
#endif

	//printf("Signal received\n");
	struct timeval eTime;
	gettimeofday(&eTime, NULL);		
	double endSeconds = eTime.tv_sec + ((double)eTime.tv_usec)/1000000.0;
	
	printf("Execution time: %f\n", endSeconds - startSeconds);
	
	char cmd[100];
	char res[100];
	sprintf(cmd, "Task %d", v->task_id);
	sprintf(res, "Runtime %f", endSeconds - startSeconds);
	reply(cmd, res);
	
	free(v);
}

void *worker_handler(void *data)
{
	struct mproc_state *mps = (struct mproc_state *) data;
	struct task_desc *task;

	printf("Started Master Process...\n");
	
#ifdef HOST_PARALLEL
	int numHostThreads = 1;
	omp_set_num_threads(numHostThreads);
	
	#pragma omp parallel
#endif
	
	while(!(*(mps->kill_master)))
	{
		
#ifdef HOST_PARALLEL
		#pragma omp critical
		{
			task = dequeue(mps->incoming);
		}
#else
		task = dequeue(mps->incoming);
#endif

		printf("Dequeued task, starting\n");
		
		struct timeval hStart, hEnd;
		gettimeofday(&hStart, NULL);
		
		task = execute_task(task);
		enqueue(task, mps->results);
		
		gettimeofday(&hEnd, NULL);
		double start = hStart.tv_sec + ((double)hStart.tv_usec)/1000000.0;
		double end = hEnd.tv_sec + ((double)hEnd.tv_usec)/1000000.0;
		printf("Host schedule time: %f\n", end-start);
		
	}

	return NULL;
}


/*
 * NOTES:
 * TODO: switch case numbers above should be in an enum
 */

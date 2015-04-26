#include "GeMTC_API.h"
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>

#define QUE_SZ  10
#define WORKERS  2
#define TASK_ID  0
#define SLEEP_DURATION 30
#define MATRIX_SIZE 8

int main(void)
{
	unsigned int *sleep_time = malloc (sizeof(unsigned int));
	unsigned int *matrix_size = malloc (sizeof(unsigned int));

	int *id = (int *) malloc(sizeof(int));
	void *params;

	*id = -1;
	*sleep_time = SLEEP_DURATION;
	*matrix_size = MATRIX_SIZE;

	gemtc_setup(QUE_SZ, WORKERS);

	//Create a thread to poll the results.
		//pthread_create((pthread_t *) malloc(sizeof(pthread_t)), NULL, &gemtc_poll, (void *)params);

//		gemtc_push(1,0, TASK_ID, (void *)sleep_time);
		gemtc_push(2,1,1, (void *)matrix_size);

		sleep(1);

		gemtc_poll(id, params);
	printf("%d\n", *id);
	
	gemtc_cleanup();
	free(sleep_time);
	free(id);

	return(0);
}

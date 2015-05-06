#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/file.h>
#include "mmap_common.h"
#include "GeMTC_API.h"
#include <assert.h>
#include <sys/time.h>

char *map_in, *map_out;
int fin, fout;
int task_id = 0;

#define QUE_SZ  1000
#define WORKERS  2
#define TASK_ID  0
#define SLEEP_DURATION 5
#define MATRIX_SIZE 8

void reply(char* command, char* status)
{
#ifdef VERBOSE
	printf("Reply:\t%s;%s\n", command, status);
#endif
	int c_len = strlen(command);
	int s_len = strlen(status);
	int write_len = c_len + s_len + 2;
	char output[write_len+1];

#ifdef VERBOSE
	printf("c_len: %d\n", c_len);
	printf("s_len: %d\n", s_len);
	printf("write_len: %d\n", write_len);
#endif

	int p = 0;
	memcpy(&output[p], command, c_len * sizeof(char));	p += c_len;
	output[p++] = ';';
	memcpy(&output[p], status, s_len * sizeof(char));	p += s_len;
	sanitizeString(output);
	output[p++] = '\n'; // newline here is important
	output[p++] = 0;

	flock(fout, LOCK_EX);
	insert(map_out, bufferSize_out, output, write_len);
	flock(fout, LOCK_UN);
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */
    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            assert(idx < count);
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }
        assert(idx == count - 1);
        *(result + idx) = 0;
    }

    return result;
}

void run(char* command)
{
//#ifdef VERBOSE
	printf("run command: \"%s\"\n", command);
//#endif
	char** params;
	params = str_split(command, ' ');

	if (params)
	{
		int i = 0;
		int task_type, num_threads;
		if(params + 1)
		{
			task_type = atoi(*(params + 1));
		}
		if(params + 2)
		{
			num_threads = atoi(*(params + 2));
		}
		int *input = malloc (sizeof(int));
		if(params + 3)
		{
			*input = atoi(*(params + 3));
		}
		//printf("Input %d\n",*input);
		
		task_id++;
#ifdef VERBOSE
		printf("Pushing task onto queue, task_type = %d, num_threads = %d, input = %d\n", task_type, num_threads, *input);
#endif

		gemtc_push(task_type, num_threads, task_id, (void *)input);
		
#ifdef VERBOSE
		printf("\n");
#endif
	}

}



int main(void)
{
	unsigned int *params = malloc (sizeof(unsigned int));
	unsigned int *matrix_size = malloc (sizeof(unsigned int));

	gemtc_setup(QUE_SZ, WORKERS);

	int i;

		int filesize_in = bufferSize_in * sizeof(char);
		int filesize_out = bufferSize_out * sizeof(char);


		fin = openFile(filePathIn, filesize_in, O_RDWR);
		map_in = mapFile(fin, filesize_in, PROT_READ | PROT_WRITE);

		fout = openFile(filePathOut, filesize_out, O_RDWR);
		map_out = mapFile(fout, filesize_out, PROT_READ | PROT_WRITE);


		printf("checking for incoming tasks\n");
		while(1){
			int len = strlen(map_in);
			if(len > 0){
				flock(fin, LOCK_EX);
				len = strlen(map_in);

				// When there's something in the buffer,
				// start splitting by newline and then run()
				// the resultant parts
				char *sourceStr, *command, *saveptr1;
				int j;
				for (j = 1, sourceStr = map_in;; j++, sourceStr = NULL) {
					command = strtok_r(sourceStr, "\n", &saveptr1);
					if (command == NULL)
					{
						break;
					}

					sanitizeString(command);
#ifdef VERBOSE
					printf("%d: %s\n", j, command);
#endif
					run(command);
				}

				for(i = 0; i < len; i++)
				{
					map_in[i] = 0;
				}
				flock(fin, LOCK_UN);
			}

			usleep(10*1000);
		}

		/* Don't forget to free the mmap_inped memory
		 */
	#ifdef VERBOSE
		printf("unmap_inping memory\n");
	#endif
		if (munmap(map_in, filesize_in) == -1) {
			perror("Error un-mmap_inping the file");
		}

		if (munmap(map_out, filesize_out) == -1) {
			perror("Error un-mmap_inping the file");
		}

		/* Un-mmap_ining doesn't close the file, so we still need to do that.
		 */
		close(fin);
		close(fout);

	gemtc_cleanup();

	return(0);
}

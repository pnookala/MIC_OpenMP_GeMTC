/*
 * client.c
 *
 *  Created on: Apr 26, 2015
 *      Author: pnookala
 */

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

// map_in is the host's input queue, etc.
char *map_in, *map_out;
int fin, fout;

char* logfile_prefix = "mic_openmp_output";
char* logfile_suffix = "out";

void runCommand(char* command)
{
	sanitizeString(command);
	int c_len = strlen(command);

	char output[c_len+2];
	memcpy(output, command, c_len * sizeof(char));
	output[c_len] = '\n';
	output[c_len+1] = 0;

	flock(fin, LOCK_EX);
	insert(map_in, bufferSize_in, output, c_len);
	flock(fin, LOCK_UN);
}

void openFileAndExecute(char* path)
{
	FILE *stream;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	stream = fopen(path, "r");
	if (stream == NULL)
	{
		exit(EXIT_FAILURE);
	}

	while ((read = getline(&line, &len, stream)) != -1)
	{
		printf("> %s", line);
		runCommand(line);
	}

	free(line);
	fclose(stream);
}

void watchOutput(){

	int i;
	// +5 = 3 digits and 2 '.'s
	int fname_len = strlen(logfile_prefix) + 5 + strlen(logfile_suffix);
	char path[fname_len];
	for(i = 0; i < 1000; i++)
	{
		sprintf(path, "%s.%03d.%s", logfile_prefix, i, logfile_suffix);
		if( access(path, F_OK) == -1){
			// file doesn't exist, use this name
			break;
		}
	}
	printf("creating output logging file: %s\n", path);
	FILE* log_out = fopen(path, "w");

	printf("starting loop\n");
	while(1){


		int len = strlen(map_out);
		if(len > 0){
			flock(fout, LOCK_EX);
			len = strlen(map_out);
			for (i = 0; i < len; i++) {
				printf("%c", map_out[i]);
				fprintf(log_out, "%c", map_out[i]);
				map_out[i] = 0;
			}
			flock(fout, LOCK_UN);
			fflush(log_out);
		}


		usleep(10*1000);
	}

	fclose(log_out); // useless? probably
}

int main(int argc, char *argv[])
{
	int filesize_in = bufferSize_in * sizeof(char);
	int filesize_out = bufferSize_out * sizeof(char);

	fin = openFile(filePathIn, filesize_in, O_RDWR);
	map_in = mapFile(fin, filesize_in, PROT_READ | PROT_WRITE);

	fout = openFile(filePathOut, filesize_out, O_RDWR);
	map_out = mapFile(fout, filesize_out, PROT_READ | PROT_WRITE);

	if(argc >= 2)
	{

		// Todo: fork(); in order to read/write to both queues

		// if parent thread
		{
			char* arg = argv[1];
			// If there is no file by this name
			if( access(arg, F_OK) == -1)
			{
				// then we have a command
				// Find overall command length
				int i, length = argc-2;
				for(i = 1; i < argc; i++)
				{
					length += strlen(argv[i]);
				}

				// combine argument parts into command
				char cmd[length]; cmd[length] = 0;
				length = 0;
				for(i = 1;;)
				{
					int s = strlen(argv[i]);
					char* currentArg = argv[i];
					memcpy( &cmd[length], currentArg, s);
					length += s;
					if(++i >= argc)
					{
						break;
					}
					cmd[length++] = ' ';
				}

				printf("running command: \"%s\"\n", cmd);
				runCommand(cmd);
			}
			else
			{
				// otherwise we have a file/script for input
				printf("executing commands in file: %s\n", arg);
				openFileAndExecute(arg);
			}
		}
		// else if child thread
		{
			//watchOutput();
		}

		// wait for child
	}
	else
	{
		printf("Client in read-only mode\n");

		watchOutput();
	}
}

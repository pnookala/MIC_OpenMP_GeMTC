/*
 * mmap_common.h
 *
 *  Created on: Apr 26, 2015
 *      Author: pnookala
 */

#ifndef SRC_MMAP_COMMON_H_
#define SRC_MMAP_COMMON_H_

#include <string.h>

// Use gcc -DVERBOSE to enable #ifdef VERBOSE logging

char* filePathIn = "/tmp/mic_openmp_shmem_in.bin";
char* filePathOut = "/tmp/mic_openmp_shmem_out.bin";
int bufferSize_in = 10000;
int bufferSize_out = 20000;

#define FILEMODE (0600)

void createFile(char* path, int size)
{
#ifdef VERBOSE
	printf("\tcreating new file\n");
#endif
	int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, (mode_t)FILEMODE);
	if (fd == -1) {
		perror("Error opening file for writing");
		exit(EXIT_FAILURE);
	}

#ifdef VERBOSE
	printf("\tinitializing file, length %d\n", size);
#endif
	/* Stretch the file size to the size of the (mmapped) array of ints
	 */
	int result;
	result = lseek(fd, size-1, SEEK_SET);
	if (result == -1) {
		close(fd);
		perror("Error calling lseek() to 'stretch' the file");
		exit(EXIT_FAILURE);
	}

	/* Something needs to be written at the end of the file to
	 * have the file actually have the new size.
	 * Just writing an empty string at the current file position will do.
	 *
	 * Note:
	 *  - The current position in the file is at the end of the stretched
	 *	file due to the call to lseek().
	 *  - An empty string is actually a single '\0' character, so a zero-byte
	 *	will be written at the last byte of the file.
	 */
	result = write(fd, "", 1);
	if (result != 1) {
		close(fd);
		perror("Error writing last byte of the file");
		exit(EXIT_FAILURE);
	}
	close(fd);
#ifdef VERBOSE
	printf("\tfile created\n");
#endif
}

// Opens the file and creates it if it doesn't exist.
// File will be created with the length specified, but
// if it exists is not guaranteed to have the proper length
int openFile(char* path, int size, int mode)
{
#ifdef VERBOSE
	printf("Opening file: %s\n", path);
#endif
	if( access(path, F_OK) == -1){
		createFile(path, size);
	}

#ifdef VERBOSE
	printf("\topening file with mode: %d\n", mode);
#endif
	int fd = open(path, mode, (mode_t)FILEMODE);
	if (fd == -1) {
		perror("Error opening file for mmap-ing");
		exit(EXIT_FAILURE);
	}
	return fd;
}

// mode will be "PROT_READ | PROT_WRITE" in most cases
char* mapFile(int fd, int fileSize, int mode)
{
#ifdef VERBOSE
	printf("mmap-ing file\n");
#endif
	char* map = mmap(0, fileSize, mode, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		perror("Error mmapping the file");
		exit(EXIT_FAILURE);
	}
	return map;
}

//int openFile(char* path, int size)
//{
//	return openFile(path, size, O_RDWR);
//}


int canPut(char* target, int capacity, int put_size)
{
	int filled = strlen(target);
	return (capacity > filled + put_size);
}
//bool insert(char* message)

void sanitizeString(char* str)
{
	int i;
	for(i = 0; str[i] > 0; i++){
		if((int)str[i] < 32){
			str[i] = ' ';
		}
	}

	for(i = strlen(str); i > 0; i--)
	{
		if((int)str[i] > 32)
			break;
		str[i] = 0;
	}
}

// puts the specified <command> in <queue>
// Warning: may cause deadlocks when queue_capacity is exceeded and
// method is forced to wait for queue to empty. Untested. In future,
// release the file lock when in the while() loop
void insert(char* queue, int queue_capacity, char* command, int dummy)
{
	int c_len = strlen(command);

#ifdef VERBOSE
	int i;
	printf("insert\n");
	printf("\tmsg: \"%s\"\n", command);
	printf("\t");
	for(i=0; i<c_len; i++)
	{
		printf("%02x", command[i]);
		if( i % 2 == 1)
			printf(" ");
	}
	printf("\n");
	printf("\tqueue len/cap: %d/%d\n", strlen(queue), queue_capacity);
	printf("\twrite length:  %d\n", c_len);
	printf("\tcan write? %d\n", canPut(queue, queue_capacity, c_len));
#endif

	while(!canPut(queue, queue_capacity, c_len))
	{
		usleep(10*1000);
#ifdef VERBOSE
		printf("\tcan write? %d\n", canPut(queue, queue_capacity, c_len));
#endif
	}

	// must be acquired after canPut allows us to write
	int queue_len = strlen(queue);

#ifdef VERBOSE
	printf("\t[%d, %d)\n", queue_len, queue_len + c_len);
#endif

	// Puts this command at the end of the queue (at queue_len)
	memcpy( &queue[queue_len], command, c_len * sizeof( char ) );
	// Ensure it's null terminated (even though it should be anyways)
	queue[queue_len + c_len] = 0;

}


#endif /* SRC_MMAP_COMMON_H_ */

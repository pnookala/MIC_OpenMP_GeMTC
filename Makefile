### Basic Makefile for OpenMP

# Offload mode execution
CC = icc -Wall -openmp -fPIC -wd2568

# Native Phi Execution
# CC = icc -mmic -g -Wall -openmp -fPIC  -L /opt/intel/lib/mic -Wno-unknown-pragmas -liomp5

# Using GCC
# CC = gcc -fopenmp


CFLAGS = -w -g -DOFFLOAD -DSINGLE_TASK
# See MasterProcess.c for additional options
# To offload to the Phi, -DOFFLOAD
# To run host scheduler multithreaded (untested), -DHOST_PARALLEL
# To block parallel tasks (only execute one at a time), -DSINGLE_TASK

LDFLAGS = $(CFLAGS)

all:   
		$(CC) $(CFLAGS) -c src/MIC_OpenMP.c src/Apps/MatrixMul.c src/Apps/Sleep.c src/MasterProcess.c src/GeMTC_API.c src/Queue.c
		$(CC) $(LDFLAGS) -o MIC_OpenMP MIC_OpenMP.o MatrixMul.o Sleep.o MasterProcess.o GeMTC_API.o Queue.o
		$(CC) $(CFLAGS) -c src/client.c
		$(CC) $(LDFLAGS) -o client client.o

MIC_OpenMP: MIC_OpenMP.o
		$(CC) $(LDFLAGS) -o MIC_OpenMP  MIC_OpenMP.o

MIC_OpenMP.o: MIC_OpenMP.c
		$(CC) $(CFLAGS) -c src/MIC_OpenMP.c src/Apps/MatrixMul.c src/Apps/Sleep.c src/MasterProcess.c src/GeMTC_API.c src/Queue.c

clean: 
		rm -f MIC_OpenMP client *.o

rebuild: clean all
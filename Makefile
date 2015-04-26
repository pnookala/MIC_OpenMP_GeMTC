### Basic Makefile for OpenMP

# Offload mode execution
 CC = icc -Wall -openmp -fPIC -wd2568
# Native Phi Execution
# CC = icc -mmic -g -Wall -openmp -fPIC  -L /opt/intel/lib/mic -Wno-unknown-pragmas -liomp5
# Using GCC
# CC = gcc -fopenmp
CFLAGS = -g
LDFLAGS =

MIC_OpenMP: MIC_OpenMP.o
		$(CC) $(LDFLAGS) -o MIC_OpenMP  MIC_OpenMP.o

MIC_OpenMP.o: MIC_OpenMP.c
		$(CC) $(CFLAGS) -c src/MIC_OpenMP.c src/Apps/MatrixMul.c src/Apps/Sleep.c src/MasterProcess.c src/GeMTC_API.c src/Queue.c

all:   
		$(CC) $(CFLAGS) -c src/MIC_OpenMP.c src/Apps/MatrixMul.c src/Apps/Sleep.c src/MasterProcess.c src/GeMTC_API.c src/Queue.c
		$(CC) $(LDFLAGS) -o MIC_OpenMP MIC_OpenMP.o MatrixMul.o Sleep.o MasterProcess.o GeMTC_API.o Queue.o

clean: 
		rm -f MIC_OpenMP MIC_OpenMP.o MatrixMul.o Sleep.o MasterProcess.o GeMTC_API.o Queue.o

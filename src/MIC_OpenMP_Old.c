/*
 ============================================================================
 Name        : MIC_OpenMP.c
 Author      : Poornima Nookala
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

//#pragma offload_attribute (push, target (mic))

#ifndef MIC_DEV
#define MIC_DEV 0

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "Apps/MatrixMul.h"
#include "omp.h"
#include <sys/time.h>
#include "float.h"
#include "GeMTC_API.h"

#define basetype int
#define basetypeprint "%d"
/* This section is for offload mode */
/*__attribute__ ((target(mic)))  int numThreads = 1, numIterations = 1;

__attribute__ ((target(mic))) void MatrixMultiplication(int sqrtElements);
__attribute__ ((target(mic))) void Sleep(int sleepTime);*/
/* This section is for native mode */
int numThreads=1, numIterations=1;

void MatrixMultiplication(int sqrtElements);
void Sleep(int sleepTime);


int main(int argc, char *argv[]) {
	/*if (argc < 4) {
		fprintf(stderr, "Expected 3 arguments, <numThreads> <numIterations> <jobType>\nUsing default arguments\n");
		//return -1;
		argv[1] = "1";
		argv[2] = "1";
		argv[3] = "1";
		argv[4] = "500";
	}
	
	numThreads = atoi(argv[1]);
	numIterations = atoi(argv[2]);
	int jobType = atoi(argv[3]);
	int duration = 500;
	if(argc >= 5){
		duration = atoi(argv[4]);
	}

	printf("Using following arguments\n");
	printf("\tnumThreads:    %i\n", numThreads);
	printf("\tnumIterations: %i\n", numIterations);
	printf("\tjobType:       %i\n", jobType);
	if(argc >= 5){
		printf("\tduration:      %i\n", duration);
	}
	printf("\n");
	
	switch (jobType) {
	case 1:
		//duration is in microseconds
		Sleep(duration);
		break;
	case 2:
		MatrixMultiplication(duration); // Not really duration, it's the number of rows and columns in the test matrix (ex, MatrixMult(5) does a 5x5 matrix.
		break;
	}*/
	gemtc_setup(10, 10);

	gemtc_cleanup();
}

 void MatrixMultiplication(int sqrtElements)
{
	printf("Starting matrix multiplication\n");
	omp_set_num_threads(numThreads);

	int dimA = 512, dimB = 216; //Size should be a multiple of 8 to avoid segmentation fault error on Xeon Phi
	if(sqrtElements>0){
		dimA = dimB = sqrtElements;
	}

	printf("\tMatrixMult, Creating matrices with dimmension %dx%d\n", dimA, dimB);
	matrix2d *A, *B, *C;
	A = createMatrix(dimA, dimB);
	B = createMatrix(dimB, dimA);
	
	printf("\tMatrixMult, Randomizing source matrices\n");
	randomizeMatrix(A);
	randomizeMatrix(B);

	printf("Matrix A:\n");
	printMatrix(A, 'd');
	printf("Matrix B:\n");
	printMatrix(B, 'd');
	//__Offload_report(2);

	printf("\tMatrixMult, Initializing MIC\n");
	int nt;
	nt = omp_get_num_threads();
double totalTime=0, minTime = DBL_MAX, maxTime = 0.;
	
//#pragma offload target(mic:MIC_DEV) \
	 in(A:length(sizeof(matrix2d*))) \
	 in(B:length(sizeof(matrix2d*))) \
	out(C:length(sizeof(matrix2d*)))
#pragma omp parallel
	{
		/* warm up to overcome setup overhead */
		printf("\tMatrixMult, Test run\n");
		C = multiplyMatrices(A, B);
		
//		double totalTime=0, minTime = DBL_MAX, maxTime = 0.;
		struct timeval tvBegin, tvEnd, tvDiff;
		/*Run matrix multiplication numIterations times and calculate the average running time. */
		
		int i;
		for (i = 0; i < numIterations; i++) {
			printf("\tMatrixMult, Starting iter: %d\n", i);
			gettimeofday(&tvBegin, NULL);
			C = multiplyMatrices(A, B);
			gettimeofday(&tvEnd, NULL);
			
			double start =  tvBegin.tv_sec + ((double)tvBegin.tv_usec/1e6);
			double end = tvEnd.tv_sec + ((double)tvEnd.tv_usec/1e6);
			double diff = end - start;
			
			maxTime = (maxTime > diff) ? maxTime : diff;
			minTime = (minTime < diff) ? minTime : diff;
			totalTime += diff;
		}
	}
	printf("\tMatrixMult, Completed\n");
	printf("Product (C):\n");
	printMatrix(C, 'd');
		
	double aveTime = totalTime / numIterations;
	long ops = C->rows * C->cols * C->rows;
	double gflops = (double)ops * (double)numIterations / ((double)(1e9) * aveTime);
	
	printf( "MatrixMult, Summary, ");
	printf( "%d threads,", numThreads);
	printf( "%d iterations,", numIterations);
	printf( "%dx%d matrix,", C->rows, C->cols);
	printf( "%g maxRT,", maxTime);
	printf( "%g minRT,",minTime);
	printf( "%g aveRT,", aveTime);
	printf( "%g totalRT,", totalTime);
	printf( "%d operations per iteration,", ops);
	printf( "%g GFlop/s\n",gflops);

	
	deleteMatrix(A);
	deleteMatrix(B);
	deleteMatrix(C);

	//free(A);
	//free(B);
	//free(C);

	//__Offload_report(2);
	return;
}



#endif

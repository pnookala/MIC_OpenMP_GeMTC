#include "stdio.h"
#include <math.h>
#include <stdlib.h>
#include "MatrixMul.h"
#include "omp.h"
#include <sys/time.h>
#include <pthread.h>

// Performs a naive multiplication of A * B (which is O(n^3)).
// If A and B are N x M and M x P respectively, the result will be an N x P matrix
// nb: N x M signifies a matrix with N rows and M columns
void *GetResponse();
matrix2d *A, *B, *C;
double totalTime=0, minTime = 0., maxTime = 0.;
struct timeval tvBegin, tvEnd, tvDiff;
int i,s1;

void MatrixMultiplication(int sqrtElements, int numThreads)
{

	printf("Starting matrix multiplication with %d elements\n", sqrtElements);
	omp_set_num_threads(numThreads);

	int dimA = 8, dimB = 8; //Size should be a multiple of 8 to avoid segmentation fault error on Xeon Phi
	if(sqrtElements>0){
		dimA = dimB = sqrtElements;
	}

	printf("\tMatrixMult, Creating matrices with dimmension %dx%d\n", dimA, dimB);

	A = createMatrix(dimA, dimB);
	B = createMatrix(dimB, dimA);
	C = createMatrix(dimA, dimB);

	printf("\tMatrixMult, Randomizing source matrices\n");
	randomizeMatrix(A);
	randomizeMatrix(B);

	printf("Matrix A:\n");
	printMatrix(A, 'd');
	printf("Matrix B:\n");
	printMatrix(B, 'd');

	printf("\tMatrixMult, Initializing MIC\n");

	gettimeofday(&tvBegin, NULL);
#pragma offload target(mic:MIC_DEV) in(A,B) out(C)  signal(s1)
#pragma omp parallel
	{
		C = multiplyMatrices(A, B);
	}
	//Spawn a new thread to wait for the results from Xeon Phi
	pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
	pthread_create(bg, NULL, GetResponse, NULL);


}


void *GetResponse()
{
#pragma offload_wait target(mic:MIC_DEV) wait(s1)
	gettimeofday(&tvEnd, NULL);

	double start =  tvBegin.tv_sec + ((double)tvBegin.tv_usec/1e6);
	double end = tvEnd.tv_sec + ((double)tvEnd.tv_usec/1e6);
	double diff = end - start;

	maxTime = (maxTime > diff) ? maxTime : diff;
	minTime = (minTime < diff) ? minTime : diff;
	totalTime += diff;
	//	}

	printf("\tMatrixMult, Completed\n");
	printf("Product (C):\n");
	printMatrix(C, 'd');

	double aveTime = totalTime / numIterations;
	long ops = C->rows * C->cols * C->rows;
	double gflops = (double)ops * (double)numIterations / ((double)(1e9) * aveTime);

	printf( "MatrixMult, Summary, \n");
	//printf( "%d threads,\n", numThreads);
	printf( "%d iterations,\n", numIterations);
	printf( "%dx%d matrix,\n", C->rows, C->cols);
	printf( "%g maxRT,\n", maxTime);
	printf( "%g minRT,\n",minTime);
	printf( "%g aveRT,\n", aveTime);
	printf( "%g totalRT,\n", totalTime);
	printf( "%d operations per iteration,\n", ops);
	printf( "%g GFlop/s\n",gflops);


	deleteMatrix(A);
	deleteMatrix(B);
	deleteMatrix(C);

	//free(A);
	//free(B);
	//free(C);

}

matrix2d* multiplyMatrices(matrix2d* A, matrix2d* B) {
	/*if(A->cols != B->rows){
		 return NULL;
		 }*/

	matrix2d* C;
	//C = malloc(sizeof(matrix2d));
	posix_memalign((void**)&C, 64, sizeof(matrix2d));

	int rows = A->rows;
	int m = A->cols;
	int cols = B->cols;
	//	int colsPadded = ( cols%8 == 0 ? cols : cols + (8-cols%8) );
	//	int rowsPadded = ( rows%8 == 0 ? rows : rows + (8-rows%8) );

	//C->data = malloc(sizeof(basetype*) * rows);
	posix_memalign((void**)&(C->data), 64, sizeof(basetype*) * rows);

#pragma offload target(mic:MIC_DEV) in(A,B) out(C)
#pragma omp parallel
	{

		// Initialize matrix rows
		C->rows = rows;
		C->cols = cols;
		//#pragma vector aligned
		//#pragma ivdep

		int r = 0;
		for (; r < rows; r++) {
			// Initialize matrix columns
			//C->data[r] = malloc(sizeof(basetype) * cols);
			posix_memalign((void**)&(C->data[r]), 64, sizeof(basetype) * cols);
			int c = 0;
			for (; c < cols; c++) {
				basetype item = 0;
				// Determine product of A's row and B's col
				int i = 0;
				for (; i < A->cols; i++) {
					item += A->data[r][i] * B->data[i][c];
				}
				// Assign to matrix
				C->data[r][c] = item;
			}
		}
	}

	return C;
}

matrix2d* loadMatrixFile(char* file) {
	FILE *fp;
	fp = fopen(file, "r");

	// Initialize matrix
	matrix2d* final;
	//	final = malloc(sizeof(matrix2d));
	posix_memalign((void**)&final, 64, sizeof(matrix2d));

	if (fp == NULL) {
		//fprintf(stderr, "Can't open input file %s\n", file);
		exit(1);
	}

	int rows, cols;
	if (fscanf(fp, "%d %d", &rows, &cols) != EOF) {
		//printf("Reading %d x %d\n", rows, cols);

		// Initialize matrix rows
		final->rows = rows;
		final->cols = cols;
		//	final->data = malloc(sizeof(basetype*) * rows);
		posix_memalign((void**)&(final->data), 64, sizeof(basetype*) * rows);

		int r = 0;
		int c = 0;
		//	for(; c < cols; c++){
		//		printf("\t[][%d]", c);
		//	}
		//	printf("\n");
		for (; r < rows; r++) {
			c = 0;
			//printf("[%d][]", r);

			// Initialize matrix columns
			//final->data[r] = malloc(sizeof(basetype) * cols);
			posix_memalign((void**)&(final->data[r]), 64, sizeof(basetype) * cols);
			for (; c < cols; c++) {
				basetype item = 0;
				if (fscanf(fp, basetypeprint, &item) == EOF) {
					printf("Unexpected EOF while reading at [%d][%d]\n", r, c);
					break;
				}
				// Assign to matrix
				final->data[r][c] = item;
				//printf("\t%d", final->data[r][c]);
			}
			//printf("\n");
		}
		//printf("\n");
	} else {
		printf("Couldn't get rows and columns");
	}

	fclose(fp);
	return final;
}

matrix2d* createMatrix(int rows, int cols) {
	//matrix2d* final = malloc(sizeof(matrix2d));
	matrix2d* final;
	posix_memalign((void**)&final, 64, sizeof(matrix2d));
	final->rows = rows;
	final->cols = cols;
	//final->data = malloc(sizeof(basetype*) * rows);
	posix_memalign((void**)&(final->data), 64, sizeof(basetype*) * rows);
	int r = 0;

#pragma omp parallel

	for (; r < rows; r++) {
		//final->data[r] = malloc(sizeof(basetype) * cols);
		posix_memalign((void**)&(final->data[r]), 64, sizeof(basetype) * cols);
		// done initializing here, this is just to clear everything to 0
		int c = 0;
		for (; c < cols; c++) {
			final->data[r][c] = 0;
		}
	}

	return final;
}

void randomizeMatrix(matrix2d* mat) {

	int r = 0;
	for (; r < mat->rows; r++) {
		int c = 0;
		for (; c < mat->cols; c++) {
			mat->data[r][c] = rand() % 100;
		}
	}
}

void printMatrix_simple(matrix2d* mat) {
	int maxRows = 20;
	int maxCols = 10;
	int r = 0;
	int c = 0;

	if (!mat) {
		printf("<null>\n\n");
		return;
	}

	for (; c < mat->cols; c++) {
		printf("\t[%d]", c);
		if(c >= maxCols){
			break;
		}
	}
	printf("\n");

	for (; r < mat->rows; r++) {
		printf("[%d]", r);
		for (c = 0; c < mat->cols; c++) {
			if (c >= maxCols) {
				if(r >= maxRows) {
					printf("\t...");
				} else {
					printf("\t...");
				}
				break;
			} else if(r >= maxRows) {
				printf("\t...");
			} else {
				printf("\t%d", mat->data[r][c]);
			}
		}

		if (r == 0) {
			printf("\t%d x %d", mat->rows, mat->cols);
		} else if (r == 1) {
			printf("\t(%d elements)", mat->rows * mat->cols);
		}
		printf("\n");

		if (r >= maxRows) {
			break;
		}
	}
	printf("\n");
}

void printMatrix_compact(matrix2d* mat) {
	printf("{");
	int r = 0;
	for (; r < mat->rows; r++) {
		if (r > 0) {
			printf(",");
		}
		printf("{");
		int c = 0;
		for (; c < mat->cols; c++) {
			if (c > 0) {
				printf(",");
			}
			printf("%d", mat->data[r][c]);
		}
		printf("}");
	}
	printf("}\n");
}

void printMatrix(matrix2d* mat, char format) {
	if (!mat) {
		printf("<null>\n\n");
		return;
	}

	if (format == 'c') {
		printMatrix_compact(mat);
	} else {
		printMatrix_simple(mat);
	}
}

void deleteMatrix(matrix2d* mat) {
	if (!mat)
		return;

	//printMatrix(mat);
	//printf("Freeing matrix\n");
	int r = 0;

	for (; r < mat->rows; r++) {
		//printf("\t[%d]/%d\n", r, mat->rows);
		if (mat->data[r]) {
			free(mat->data[r]);
		} else {
			//printf("was null\n");
		}
	}
	//printf("\tmat->data\n");
	free(mat->data);
	//printf("\tmat\n");
	free(mat);

}

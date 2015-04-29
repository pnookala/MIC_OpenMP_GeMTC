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

void *GetMMResponse(void *data);

double totalTime=0, minTime = 0., maxTime = 0.;
struct timeval tvBegin, tvEnd, tvDiff;
int i;

/*__attribute__ ((target(mic))) basetype *A, *B, *C;*/
__attribute__ ((target(mic))) int
	a_rows, a_cols,
	b_rows, b_cols,
	c_rows, c_cols;

basetype *A, *B, *C;
/*int     a_rows, a_cols,
        b_rows, b_cols,
        c_rows, c_cols;
*/	
void MatrixMultiplication(int sqrtElements, int numThreads, int task_id)
{
	
	printf("Starting matrix multiplication with %d elements\n", sqrtElements);
	omp_set_num_threads(numThreads);
	
	int dimA = 8, dimB = 8; //Size should be a multiple of 8 to avoid segmentation fault error on Xeon Phi
	if(sqrtElements>0){
		dimA = dimB = sqrtElements;
	}

	printf("\tMatrixMult, Creating matrices with dimmension %dx%d\n", dimA, dimB);
	a_rows = dimA;
	a_cols = dimB;
	b_rows = dimB;
	b_cols = dimA;
	
	
	A = createMatrix(a_rows, a_cols);
	B = createMatrix(b_rows, b_cols);
	//C = createMatrix(dimA, dimB);

	printf("\tMatrixMult, Randomizing source matrices\n");
	randomizeMatrix(A, a_rows, a_cols);
	randomizeMatrix(B, b_rows, b_cols);

	printf("Matrix A:\n");
	//printMatrix(A, a_rows, a_cols, 'c');
	printMatrix(A, a_rows, a_cols, 'd');
	printf("Matrix B:\n");
	//printMatrix(B, b_rows, b_cols, 'c');
	printMatrix(B, b_rows, b_cols, 'd');

	gettimeofday(&tvBegin, NULL);
	printf("\tMatrixMult, start\n");

//#pragma offload target(mic:MIC_DEV) in(A, B, a_rows, a_cols, b_rows, b_cols) out(C) signal(s1)
//#pragma omp parallel
//	{
		C = multiplyMatrices(A, B, a_rows, a_cols, b_rows, &c_rows, &c_cols, task_id);
//	}
	
	//Spawn a new thread to wait for the results from Xeon Phi
//	pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
//	pthread_create(bg, NULL, GetMMResponse, NULL);
	
}

// c_rows and c_cols are outputs, call as multiplyMatrices(... , &c_rows, &c_cols);
// If A and B are N x M and M x P respectively, the result will be an N x P matrix
basetype* multiplyMatrices(basetype* A, basetype* B, int a_rows, int a_cols, int b_cols, int* c_rows, int* c_cols, int task_id)
{
	*c_rows = a_rows;
	*c_cols = b_cols;
	
	printf("\tMatrixMult, allocating C\n");
	basetype* C;
	//posix_memalign((void**)&C, 64, sizeof(matrix2d));
	C = createMatrix(a_rows, b_cols);

	int rows = a_rows;
	int m = a_cols; // m is the "inner" and common dimension between A and B
	int cols = b_cols;
	int s1 = task_id;
//#pragma offload_transfer target(mic:MIC_DEV) in(A,B : length(sizeof(int)*a_rows*a_cols))

	printf("\tMatrixMult, Initializing MIC\n");
	 gettimeofday(&tvBegin, NULL);
//#pragma offload target(mic:MIC_DEV) \
	in(a_rows,a_cols,b_rows,b_cols,m,rows,cols) \
	 in(A,B) \
	 inout(C) signal(s1)
#pragma offload target(mic:MIC_DEV) in(A : length(a_rows*a_cols) align(4)) in(B : length(b_rows*b_cols) align(4)) out(C : length(a_rows*b_cols) align(4)) in(a_rows, a_cols, b_rows, b_cols,m,rows,cols) signal(s1)
#pragma omp parallel
	{
//#pragma omp single
//		{
		int r = 0;
		for (; r < rows; r++) {
			// Initialize matrix columns
			int c = 0;
			for (; c < cols; c++) {
				int item = 0;
				// Determine product of A's row and B's col
				int i = 0;
				for (; i < m; i++) {
					//item += A->data[r][i] * B->data[i][c];
					item += A[(r * a_cols) + i] * B[(i * b_cols) + c];
					
					//int v = A[(r * a_cols) + i] * B[(i * b_cols) + c];
					//printf("\tA[%d, %d] * B[%d, %d] = %d * %d = %d\n", r, i, i, c, A[(r * a_cols) + i], B[(i * b_cols) + c], v);
				}
				// Assign to matrix
				//C->data[r][c] = item;
				C[(r * cols) + c] = item;
				
				//printf("\t\tC[%d, %d]: %d\n", r, c, item);
			}
		}
//		}
	}
	
//	printf("Matrix C:\n");
//	printMatrix(C, *c_rows, *c_cols, 'c');
//	printMatrix(C, *c_rows, *c_cols, 'd');
	
	//Spawn a new thread to wait for the results from Xeon Phi
	int *localCopy = (int*)malloc(sizeof(int));
	*localCopy = s1;
	printf("Local copy: %d\n",*localCopy);
	pthread_t bg = (pthread_t ) malloc(sizeof(pthread_t));
	pthread_create(bg, NULL, GetMMResponse, (void *)localCopy);
	
	return C;
}

void *GetMMResponse(void *data)
{
	printf("Waiting for signal\n");
	int *v = (int *)data;
	int value = *v;
	printf("Value v: %d\n", value);
#pragma offload_wait target(mic:MIC_DEV) wait(value)

	gettimeofday(&tvEnd, NULL);

	double start =  tvBegin.tv_sec + ((double)tvBegin.tv_usec/1e6);
	double end = tvEnd.tv_sec + ((double)tvEnd.tv_usec/1e6);
	double diff = end - start;

	//maxTime = (maxTime > diff) ? maxTime : diff;
	//minTime = (minTime < diff) ? minTime : diff;
	//totalTime += diff;

	printf("\tMatrixMult, Completed\n");
	printf("Product (C):\n");
	//printMatrix(C, c_rows, c_cols, 'c');
	printMatrix(C, a_rows, b_cols, 'd');

	//	double aveTime = totalTime / numIterations;
	long ops = c_rows * c_cols * c_rows;
	//	double gflops = (double)ops * (double)numIterations / ((double)(1e9) * aveTime);

	printf( "MatrixMult, Summary, \n");
	//printf( "%d threads,\n", numThreads);
	//printf( "%d iterations,\n", numIterations);
	printf( "%dx%d matrix,\n", c_rows, c_cols);
	printf( "Execution Time = %g,\n", diff);
	//printf( "%g minRT,\n",minTime);
	//printf( "%g aveRT,\n", aveTime);
	//printf( "%g totalRT,\n", totalTime);
	//printf( "%d operations per iteration,\n", ops);
	//printf( "%g GFlop/s\n",gflops);


//	deleteMatrix(A);
//	deleteMatrix(B);
//	deleteMatrix(C);
	printf("returning\n");
}

basetype* loadMatrixFile(char* file) {
	/*
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
	*/
	return 0;
}

basetype* createMatrix(int rows, int cols) {
	basetype* final;
	//posix_memalign((void**)&final, 64, sizeof(matrix2d));
	final = malloc(rows * cols * sizeof(basetype));
	
	return final;
}

void randomizeMatrix(basetype* mat, int rows, int cols) {
	int r = 0;
	for (; r < rows; r++) {
		int c = 0;
		for (; c < cols; c++) {
			//mat->data[r][c] = rand() % 100;
			mat[(r * cols) + c] = rand() % 4;
		}
	}
}

void printMatrix_simple(basetype* mat, int rows, int cols) {
	int maxRows = 20;
	int maxCols = 10;
	int r = 0;
	int c = 0;
	
	if (!mat) {
		printf("<null>\n\n");
		return;
	}
	
	for (; c < cols; c++) {
		printf("\t[%d]", c);
		if(c >= maxCols){
			break;
		}
	}
	printf("\n");

	for (; r < rows; r++) {
		printf("[%d]", r);
		for (c = 0; c < cols; c++) {
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
				//printf("\t%d", mat->data[r][c]);
				printf("\t%d", mat[(r * cols) + c]);
			}
		}

		if (r == 0) {
			printf("\t%d x %d", rows, cols);
		} else if (r == 1) {
			printf("\t(%d elements)", rows * cols);
		}
		printf("\n");
		
		if (r >= maxRows) {
			break;
		}
	}
	printf("\n");
	
}

void printMatrix_compact(basetype* mat, int rows, int cols) {
	printf("{");
	int r = 0;
	for (; r < rows; r++) {
		if (r > 0) {
			printf(",");
		}
		printf("{");
		int c = 0;
		for (; c < cols; c++) {
			if (c > 0) {
				printf(",");
			}
			//printf("%d", mat->data[r][c]);
			printf("%d", mat[(r * cols) + c]);
		}
		printf("}");
	}
	printf("}\n");
}

void printMatrix(basetype* mat, int rows, int cols, char format) {
	if (!mat) {
		printf("<null>\n\n");
		return;
	}

	if (format == 'c') {
		printMatrix_compact(mat, rows, cols);
	} else {
		printMatrix_simple(mat, rows, cols);
	}
}

void deleteMatrix(basetype* mat) {
	if (!mat)
		return;

	printf("Freeing matrix 0x%08x\n", mat);	
	free(mat);
}

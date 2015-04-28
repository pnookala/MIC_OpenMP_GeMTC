/*
 * MatrixMul.h
 *
 *  Created on: Apr 8, 2015
 *      Author: pnookala
 */

#ifndef MATRIXMUL_H_
#define MATRIXMUL_H_
#endif 

#ifndef MIC_DEV
#define MIC_DEV 0
#endif

#define basetype int
#define basetypeprint "%d"

/* This code is for offload execution */
__attribute__ ((target(mic))) 	basetype* loadMatrixFile(char* file);
__attribute__ ((target(mic))) 	basetype* createMatrix(int rows, int cols);
__attribute__ ((target(mic))) 	void randomizeMatrix(basetype* mat, int rows, int cols);

__attribute__ ((target(mic))) 	basetype* multiplyMatrices(basetype* A, basetype* B, int a_rows, int a_cols, int b_cols, int* c_rows, int* c_cols);
//__attribute__ ((target(mic))) 	basetype* multiplyMatrices(basetype* A, basetype* B, int a_rows, int a_cols, int b_cols);

__attribute__ ((target(mic))) 	void printMatrix(basetype* mat, int rows, int cols, char format);
__attribute__ ((target(mic))) 	void deleteMatrix(basetype* mat);
__attribute__ ((target(mic)))	void printMatrix_compact(basetype* mat, int rows, int cols);
__attribute__ ((target(mic)))	void printMatrix_simple(basetype* mat, int rows, int cols);
__attribute__ ((target(mic)))	void MatrixMultiplication(int sqrtElements, int numThreads);


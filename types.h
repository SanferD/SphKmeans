#ifndef _PARAMS_H
#define _PARAMS_H

#include <stdlib.h>

/**
 * K 		Number of cluster
 * T 		Number of iterations
 * docs 	Number of docs
 * trueK	Number of classes
 * ifile_nm Name of input file
 * cfile_nm Name of class file
 * ofile_nm Name of output file
 */
typedef struct {
  int K, T, docs, dims, trueK;
  char *ifile_nm, *cfile_nm, *ofile_nm;
} Params;

/**
 * dnorms 		Array of document lengths
 * cnorms 		Array of centroid lengths
 * labels 		Array of assigned clusters
 * docids 		Array of document ids
 * classes 		Array of classes for each document
 * best 		Array of best found labels
 * K 			Same as Params.K: number of clusters
 */
typedef struct {
  double *dnorms, *cnorms;
  int *labels, *docids, *classes, *best;
  int **C;
  int K;
} Aux;


/**
 * value 		Array of values in csr matrix
 * row_ptr		Array of begining row values (size = CSR.N)
 * col_idx 		Array of column indeces corresponding to each value
 * N 			Number of rows (Params.docs+1)
 */
typedef struct {
  int *value, *row_ptr, *col_idx, N;
} CSR;

/**
 * seed			The seed which the current object represents
 * obj 			The value of the objective function
 * entropy		The corresponding entropy value
 * purity 		The corresponding purity value
 */
typedef struct {
	int seed;
	double obj, entropy, purity;
} Stats;

void free_params(Params p);
void free_aux(Aux a);
void free_csr(CSR csr);

#endif

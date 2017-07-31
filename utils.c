#include "utils.h"
#include "wrappers.h"

#include <assert.h>
#include <math.h>
#include <string.h>

/**
 * Parses the command line and returs the filled parameter structure
 * @param  args pass this directly from main
 * @return      the parsed parameters
 */
Params parse_cmdln(char *args[]) {
  Params p;
  p.ifile_nm = strdup(args[1]);
  p.cfile_nm = strdup(args[2]);
  p.K = atoi(args[3]);
  p.T = atoi(args[4]);
  p.ofile_nm = strdup(args[5]);

  return p;
}

/**
 * Reads the input files and fills out the given structures.
 * See types.h to better understand what these are
 * @param params the input parameters
 * @param aux    data structure to hold auxiliary data like norms
 * @param csr    sparse matrix where the rows are document vectors
 */
void read_input(Params *params, Aux *aux, CSR *csr) {
  int j, k;

  /* open the input file */
  FILE *fp = Fopen(params->ifile_nm, "r");

  /* first pass: count the number of NNZ */
  int NNZ = 0, docs=0; size_t len;
  char *token, *line=NULL;
  while(getline(&line, &len, fp)!=-1) {
    for(token = strtok(line, ","); token!=NULL; token=strtok(NULL,",")) NNZ++;
    free(line); line=NULL;
    docs++;
  }
  free(line); line=NULL;
  NNZ /= 3;

  /* second pass: build the CSR matrix */
  /* rewind the file */
  rewind(fp);

  /* allocate memory */
  int *value = csr->value = (int*) Malloc(sizeof(int)*NNZ);
  int *col_idx = csr->col_idx = (int*) Malloc(sizeof(int)*NNZ);
  int *row_ptr = csr->row_ptr = (int*) Malloc(sizeof(int)*(docs+1));

  /* build the CSR matrix */
  int dims = 0;
  k = j = row_ptr[0] = 0;
  while(getline(&line, &len, fp)!=-1) {
    for(token = strtok(line, ","); token!=NULL; j++, token=strtok(NULL,",")) {
      int col = atoi( token=strtok(NULL, ",") );
      int val = atoi( token=strtok(NULL, ",") );

      assert(j<NNZ);
      col_idx[j] = col;
      value[j] = val;

      if (col > dims) dims = col;
    }

    row_ptr[++k] = j;
    assert(k<=docs);
    free(line); line=NULL;
  }
  free(line); line=NULL;

  csr->N = docs;
  assert(j==NNZ);
  assert(row_ptr[k]==NNZ);
  assert(k==docs);

  /* done with building CSR matrix so close the file */
  Fclose(fp);

  params->docs = docs;
  params->dims = ++dims;

#ifdef DEBUG
  printf("docs: %i\n", docs);
  printf("dims: %i\n", dims);
  printf("NNZ: %i\n", NNZ);
#endif

  /* allocate memory for remaining auxiliaries  */
  int K = aux->K = params->K;
  double *dnorms = aux->dnorms = (double*) Malloc(sizeof(double)*docs);

  /* fill in the norm values */
  for (j=0; j!=docs; j++) {
    int sum=0;
    for (k=row_ptr[j]; k!=row_ptr[j+1]; k++)
      sum += value[k]*value[k];
    dnorms[j] = sqrt((double) sum);
  }

  /* get the true classes */
  /* .class File  */
  fp = Fopen(params->cfile_nm, "r");

  /* parse the first line: get the number of classes */
  int trueK = params->trueK = 20;

  /* allocate memory */
  int *docids = aux->docids = (int*) Malloc(sizeof(int)*docs);
  int *classes = aux->classes = (int*) Malloc(sizeof(int)*docs);
  char **topics = (char**) Malloc(sizeof(char*)*trueK);
  for (j=0; j!=trueK; topics[j++]=NULL);

  /* assign each document to its true class */
  int ins;
  for (j=0; getline(&line, &len, fp)!=-1; line=NULL, j++) {
    assert(j<docs);
    char t[256];
    sscanf(line, "%i,%s\n", &docids[j], t);

    for (ins=1, k=0; k!=trueK && topics[k]; k++)
      if (strcmp(topics[k], t) == 0) {
        classes[j] = k;
        ins = 0;
      }

    if (ins) {
      assert(k<trueK);
      topics[k] = strdup(t);
      classes[j] = k;
    }

    free(line);
  }
  free(line);

  /* clean-up */
  for (j=0; j!=trueK; free( topics[j++] ));
  free(topics);
  Fclose(fp);

  /* allocate memory for the arrays which will be filled later by the algorithm */
  aux->cnorms = (double*) Malloc(sizeof(double)*K);
  aux->labels = (int*) Calloc(docs, sizeof(int)); // 0 initialized
  aux->best = (int*) Calloc(docs, sizeof(int)); // 0 initialized

  int **C = (int **) Malloc(sizeof(int*)*K);
  for (k=0; k!=K; k++)
    C[k] = (int*) Calloc(dims, sizeof(int));
  aux->C = C;
}

/**
 * Saves the clusters to the output file
 * @param fn     Name of the outfile
 * @param ids    The array of document ids
 * @param labels The corresponding array of cluster labels
 * @param N      The size of each array
 */
void save_clusters(char *fn, int *ids, int *labels, int N) {
  FILE *fp = Fopen(fn, "w");
  int i;
  for (i=0; i!=N; i++)
    fprintf(fp, "%i,%i\n", ids[i], labels[i]);
  Fclose(fp);
}

/**
 * Perform SpmV and put the value into the result array
 * @param csr    sparse input matrix
 * @param vec    dense input vector
 * @param result dense output vector such that vec = csr x vec
 */
void mat_vec(CSR csr, int *vec, int *result) {
  int *row_ptr = csr.row_ptr, *col_idx = csr.col_idx, *value = csr.value;
  int N = csr.N;

  int i, j, sum; // assumes row-centroid dot-prod is small << INT_MAX
  for (i=0; i!=N; result[i++]=sum)
    for (sum=0, j=row_ptr[i]; j!=row_ptr[i+1]; j++)
      sum += value[j]*vec[ col_idx[j] ];
}

/**
 * Calculate the length of the vector.
 * Uses a trick to avoid oveflow.
 * http://cs-technotes.blogspot.com/2012/08/compute-euclidean-norm-for-very-large.html
 * @param  vec the vector whose length needs to be calculated
 * @param  N   the number of dimensions of the vector
 * @return     L2 norm of the vector
 */
double norm(int *vec, int N) {
  int i, max;

  for (max=i=0; i!=N; i++) if (vec[i]>max) max=vec[i];
  
  double sum = 0.0;
  for (i=0; i!=N; i++)
    sum += (double)(vec[i]*vec[i]) / (double)(max*max);

  return max*sqrt((double) sum);
}

/**
 * Copies the row of csr into the array arr.
 * Assumes 1. arr has length = # of dims
 *         2. arr[i]=0 for all i<dims
 * @param csr      document matrix
 * @param row      the row to slice
 * @param arr      the output arr
 */
void slice_row(CSR csr, int row, int *arr) {
  int *value = csr.value, *col_idx = csr.col_idx, *row_ptr = csr.row_ptr;

  int i;
  for (i=row_ptr[row]; i!=row_ptr[row+1]; i++)
    arr[ col_idx[i] ] = value[i];
}

/**
 * Add the row in csr to the vector in vec. Equiv to vec += csr[row][:]
 * @param vec the vector to which to add the row of csr matrix
 * @param csr the sparse matrix of document vectors
 * @param row the row to add to the vector
 */
void add_vec(int *vec, CSR csr, int row) {
  int *row_ptr=csr.row_ptr, *col_idx=csr.col_idx, *value=csr.value;

  int i;
  for (i=row_ptr[row]; i!=row_ptr[row+1]; i++)
    vec[ col_idx[i] ] += value[i];
}
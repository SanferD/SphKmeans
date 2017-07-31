#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "utils.h"
#include "wrappers.h"

int cmp (const void * e1, const void * e2);
Stats compute_ep_stats(Aux aux, int N, int K, int trueK);
void print_matrix(int *clusters, int *classes, int docs, int n_clusters, int n_classes);
double run_spherical_k_means(Params params, Aux aux, CSR csr, int seed);
void shuffle(int *array, int n);

#ifdef DEBUG
  int iter_tot=0;
#endif



int main(int argc, char *argv[]) {
  /* parse the command line input */
  if (argc != 6) {
    printf("usage: ./sphkmeans <input-file> <class-file> <#clusters> <#trials> <output-file>\n");
    return -1;
  }

  /* get the cmdline input */
  Params params = parse_cmdln(argv);
  if (params.T < 0) params.T = 0; else if (params.T > 20) params.T = 20;
#ifdef DEBUG
  printf("%s %s %s K:%i T:%i\n", params.ifile_nm, params.cfile_nm, params.ofile_nm, params.K, params.T);
#endif

  /* parse the file into a CSR matrix */
  Aux aux;
  CSR csr;
  read_input(&params, &aux, &csr);

#ifdef DEBUG
  /* run the trials */
  clock_t start = clock();
#endif

  int i, seeds[] = {1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39};
  Stats best = {.obj = -1.0, .entropy = 1.0, .purity = -1.0, .seed = -1};
  for (i=0; i!=params.T; i++) {
		int seed = seeds[i];

  	/* run the K means algorithm */
	  double obj;
	  obj = run_spherical_k_means(params, aux, csr, seed);

	  /* compute entropy and purity stats */
	  Stats s = compute_ep_stats(aux, params.docs, params.K, params.trueK);
  	
  	/* keep only the best values */
	  if (obj > best.obj) { 
      best.obj = obj; 
      best.entropy = s.entropy; 
      best.purity = s.purity; 
      best.seed = seed; 
    }
  }

#ifdef DEBUG
  /* print the time */
  printf("time: %.3f\n", (double) (clock()-start) / CLOCKS_PER_SEC);
  printf("avg iter: %.3f\n", (double) iter_tot / (double) params.T);
#endif

  /* print the best stats */
  printf("obj: %.3f\n", best.obj);
  printf("entropy: %.3f\n", best.entropy);
  printf("purity: %.3f\n", best.purity);
  
  /* print the cluster matrix to stdout */
  print_matrix(aux.best, aux.classes, params.docs, params.K, params.trueK);

  /* write the best solution to the output file */
  save_clusters(params.ofile_nm, aux.docids, aux.best, params.docs);

  /* cleanup */
  free_aux(aux);
  free_csr(csr);
  free_params(params);

  return 0;
}



/**
 * Runs the spherical k means algorithm until no centroids move.
 * Empty clusters are initialized to a random document in the largest cluster
 * @param  params The parameter struct
 * @param  aux    The auxiliary struct
 * @param  csr    The sparse document matrix
 * @param  seed   The input value seed
 * @return        The best value for the object function for the given seed
 */
double run_spherical_k_means(Params params, Aux aux, CSR csr, int seed) {
  int i, j;

  int K=params.K, docs=params.docs, dims=params.dims, *labels=aux.labels, **C=aux.C;
  int *best=aux.best;
  double *cnorms = aux.cnorms, *dnorms = aux.dnorms;

  /* get the initial centroid points */
  int *count, *indeces;
  count = indeces = (int*) Malloc(sizeof(int)*K);

  srand(seed);
  for (i=0; i!=K; i++)
    indeces[i] = rand()%docs;

  int redo;
  do {
    redo = 0;
    qsort(indeces, K, sizeof(int), cmp);
    for (i=0; i!=K-1; i++)
      if (indeces[i]==indeces[i+1]) {
        indeces[i] = rand()%docs;
        redo = 1;
      }
  } while (redo != 0);

  /* set the count initially to 0 */
  /* initialize each centroid to the 0 vector */
  for (i=0; i!=K; i++) for (j=0; j!=dims; j++) C[i][j] = 0;

  /* slice the centroids from the csr matrix and compute their norms */
  for (i=0; i!=K; i++) {
    slice_row(csr, indeces[i], C[i]);
    cnorms[i] = norm(C[i], dims);
  }

  /* memory to store the similarity for each document to each centroid */
  /* one similiarity measure per document so dense vector */
  int **prod = (int**) Malloc(sizeof(int*)*K);
  for (i=0; i!=K; i++)
    prod[i] = (int*) Malloc(sizeof(int)*docs);

  /* zero-initialize count array */
  for (i=0; i!=K; i++) count[i] = 0;

  /* find local minima */
  int try_again; double best_obj=-INFINITY; int update_best;
#ifdef DEBUG
  int iter=0;
#endif
  do {
#ifdef DEBUG
    iter++;
#endif
    /* compute similarity between each centroid */
    for (i=0; i!=K; i++)
      mat_vec(csr, C[i], prod[i]);

    /* get closest cluster for each document */
    double max, obj=0.0;
    for (try_again=i=0; i!=docs; i++) {
      int l = -1; max=-INFINITY;

      /* find closest cluster j to document i with similarity max */
      for (j=0; j!=K; j++) {
        double sim = (double) prod[j][i] / (double) (cnorms[j]*dnorms[i]);
        if (sim > max) {
          max = sim;
          l = j;
        }
      }
      assert(l != -1);
      try_again = try_again || (labels[i]!=l);
      labels[i]=l;
      count[l]++;

      assert(max<=1.1);
      obj += max;
    }
    if (obj > best_obj) {
      update_best = 1;
    	best_obj = obj;
    	memcpy(best, labels, docs*sizeof(int));
    }
    else
      update_best = 0;

    /* recompute centroids only if some of the documents moved between clusters */
    int has_empty = 0;
    for (i=0; i!=K && !has_empty; i++) has_empty = has_empty || count[i]==0; // check for any empty clusters
    
    if (try_again!=0 || has_empty!=0) {
      /* check for empty clusters */
      int *largest=0, i_largest, sz;
      if (has_empty != 0)
        for (i=0; i!=K; i++)
          if (count[i] == 0) {
            
            /* compute the array of document indexes belonging to the largest cluster */
            if (largest == 0) {
              // 1. get the size and index of the largest cluster
              sz=-1;
              for (j=0; j!=K; j++) if (count[j] > sz) sz = count[ i_largest=j ];

              // 2. find and store all document ids that belong to cluster i_largest
              largest = (int*) Malloc(sizeof(int)*sz);
              int k;
              for (k=j=0; j!=docs; j++) if (labels[j]==i_largest) largest[k++] = j;
              assert(k == sz);
              shuffle(largest, sz);
            }

            /* randomly pick a document from the largest cluster and assign to empty cluster */
            for (j=0; j!=sz/2; j++) {
              labels[ largest[j] ] = i;
              if (update_best)
                best[ largest[j] ] = i;
            }
          }
      if (largest)
        free(largest);

	    /* compute new centroids */
      // 1. zero-initialize each centroid and count vector
	    for (i=0; i!=K; i++) for (j=0; j!=dims; j++) C[i][j] = 0;
      for (i=0; i!=K; count[i++]=0) for (j=0; j!=docs; j++) prod[i][j] = 0;
      // 2. add each document belonging to the cluster labels[i]
	    for (i=0; i!=docs; i++)
	      add_vec( C[ labels[i] ], csr, i );
      // 3. calculate norm of the summed vectors
	    for (i=0; i!=K; i++)
	    	cnorms[i] = norm(C[i], dims);
    }
  } while (try_again!=0);

#ifdef DEBUG
  iter_tot += iter;
#endif

  for (i=0; i!=K; i++) free(prod[i]);
  free(prod);
  free(count);

	return best_obj;
}



/**
 * Compute the entropy and purity statistics
 * @param  aux   auxiliary struct 
 * @param  docs  number of docs
 * @param  K     number of clusters
 * @param  trueK number of classes
 * @return       struct with entropy and purity
 */
Stats compute_ep_stats(Aux aux, int docs, int K, int trueK) {
	int i, j, x;
	int *best=aux.best, *classes=aux.classes;

	/* m_ij mat where i->class and j->cluster */
	int **m = (int **) Malloc(sizeof(int*)*trueK);
	for (i=0; i!=trueK; i++)
		m[i] = (int*) Calloc(K, sizeof(int)); // 0 initialized

	/* m_j array where j->cluster */
	int *count = (int *) Calloc(K, sizeof(int)); // 0 initialized

	/* fill them */
	for (x=0; x!=docs; x++) {
		i=classes[x]; // class which doc x belongs to
		j=best[x]; // cluster which doc x belongs to
		m[i][j] += 1;
		count[j] += 1; 
	}

	/* calculate the entropy and purity array */
	double *e = (double*) Calloc(sizeof(double), K); // 0 initialized
	double *purity = (double*) Calloc(sizeof(double), K); // 0 initialized
	for (j=0; j!=K; j++)
		if (count[j]!=0)
			for (i=0; i!=trueK; i++) {
        if (m[i][j] !=0 ) {
  				double p = (double) m[i][j] / (double) count[j];
          double prod = p*(-log(p)/log(2.0));
          e[j] += prod;
  				if (purity[j] < p)
  					purity[j] = p;
        }
			}

	/* calculate the total entropy */
	double total_e = 0.0;
	for (j=0; j!=K; j++)
		total_e += (count[j]/(double) docs) * e[j];

	/* calculate the total purity */
	double total_purity = 0.0;
	for (j=0; j!=K; j++)
		total_purity += (count[j]/(double) docs) * purity[j];

	/* cleanup */
	for (i=0; i!=trueK; i++) free(m[i]);
	free(m);
	free(count);
	free(e);
	free(purity);

	Stats s = { .entropy=total_e, .purity=total_purity };
	return s;
}



/* used by qsort */
int cmp (const void * e1, const void * e2)
{
  int x = *((int*) e1);
  int y = *((int*) e2);
  if (x > y) return  1;
  if (x < y) return -1;
  return 0;
}

/**
 * Shuffles the input array in place.
 * http://stackoverflow.com/a/6127606/4646773
 * @param array	the array to shuffle
 * @param n	the number of elements in the array
 */
void shuffle(int *array, int n) {
  int i;
  for (i=0; i!=n; i++) {
    int j = i + rand() / ( RAND_MAX / (n-i) + 1 );
    int t = array[j];
    array[j] = array[i];
    array[i] = t;
  }
}

/**
 * Prints the count matrix to stdout
 * @param clusters   the array of cluster labels (size==docs)
 * @param classes    the array of class labels (size==docs)
 * @param docs       the number of documents
 * @param n_clusters number of unique cluster labels
 * @param n_classes  number of unique class labels
 */
void print_matrix(int *clusters, int *classes, int docs, int n_clusters, int n_classes) {
  int i, j;
  /* allocate memory for the output matrix */
  int ** count = (int **) Malloc(sizeof(int*)*n_clusters);
  for (i=0; i!=n_clusters; i++)
    count[i] = (int *) Calloc(n_classes, sizeof(int));

  /* fill it up */
  for (i=0; i!=docs; i++)
   count[ clusters[i] ][ classes[i] ]++;

  /* print to stdout */
  for (i=0; i!=n_clusters; i++) {
    for (j=0; j!=n_classes; j++)
      printf("%5i ", count[i][j]);
    printf("\n");
  }

  /* clean up */
  for (i=0; i!=n_clusters; i++) free(count[i]);
  free(count);
}

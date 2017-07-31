#ifndef _UTILS_H
#define _UTILS_H

#include "types.h"

Params parse_cmdln(char *args[]);
void read_input(Params *params, Aux *aux, CSR *csr);
void save_clusters(char *fn, int *ids, int *labels, int N);

void slice_row(CSR csr, int row, int *centroid);

double norm(int *vec, int N);

void mat_vec(CSR csr, int *vec, int *result);
void add_vec(int *vec, CSR csr, int row);


#endif

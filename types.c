#include "types.h"

void free_params(Params p) {
	free(p.ifile_nm);
	free(p.cfile_nm);
	free(p.ofile_nm);
}

void free_aux(Aux a) {
	free(a.dnorms);
	free(a.cnorms);
	free(a.labels);
	free(a.docids);
	free(a.classes);
	free(a.best);

	int i;
	for (i=0; i!=a.K; i++)
		free(a.C[i]);
	free(a.C);
}

void free_csr(CSR csr) {
  free(csr.row_ptr);
  free(csr.col_idx);
  free(csr.value);
}
#ifndef _WRAPPERS_H
#define _WRAPPPERS_H

#include <stdio.h>
#include <stdlib.h>

void *Malloc(size_t n);
void *Calloc(size_t num, size_t size);
FILE *Fopen(char *n, char *m);
void Fclose(FILE *fp);

#endif

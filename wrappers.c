#include "wrappers.h"

#include <stdio.h>
#include <stdlib.h>

void *Malloc(size_t n) {
  void *p = malloc(n);
  if (!p) {
    printf("malloc error\n");
    exit(1);
  }
  return p;
}

void *Calloc(size_t num, size_t size) {
  void *p = calloc(num, size);
  if (!p) {
    printf("calloc error\n");
    exit(1);
  }
  return p;
}


FILE *Fopen(char *n, char *m) {
  FILE *fp = fopen(n, m);
  if (fp == NULL) {
    char msg[256];
    sprintf(msg, "Fopen error \'%s\'", n);
    perror(msg);
    exit(1);
  }
  return fp;
}

void Fclose(FILE *fp) {
  if (fclose(fp) != 0) {
    perror("Fclose error");
    exit(1);
  }
}

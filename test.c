#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "myrand.h"

int main() {
    mysrand(49);
    printf("%i\n", myrand() % 8000);
    printf("%i\n", myrand() % 8000);
    printf("%i\n", myrand() % 8000);
    printf("%i\n", myrand() % 8000);
    printf("%i\n", myrand() % 8000);

}
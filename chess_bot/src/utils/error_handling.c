#include <stdio.h>
#include <stdlib.h>

#include "utils/error_handling.h"

void eh_die(char *msg){
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/transposition_table.h"

#define NUM_ITEMS 100000

int main(int argc, char *argv[]){
    if (argc == 2){
        char *end_ptr;
        unsigned long seed = strtoul(argv[1], &end_ptr, 10);
        if (*end_ptr != '\0' || *argv[1] == '-'){
            fprintf(stderr, "Usage: tt_test [seed]\nseed: positive integer that is the seed the program should use\n");
            return 1;
        }
        srand((unsigned int)seed);
    }
    else if (argc > 2){
        fprintf(stderr, "Usage: tt_test [seed]\nseed: positive integer that is the seed the program should use\n");
        return 1;
    }
    else{
        srand(1);
    }

    _transposition_table *table = tt_create_transposition_table(sizeof(int));

    // time to store items
    uint64_t *keys = malloc(sizeof(uint64_t) * NUM_ITEMS);
    if (keys == NULL){
        tt_destroy_transposition_table(table);
        printf("malloc() failed\n");
        return 1;
    }
    clock_t start = clock();
    for (int i = 0; i < NUM_ITEMS; i++){
        keys[i] = (uint64_t)rand();
        tt_insert_item(table, keys[i], &i);
    }
    clock_t end = clock();
    printf("time to insert %d entries: %f seconds\n", NUM_ITEMS, ((double) (end - start)) / CLOCKS_PER_SEC);

    // time to retrieve items
    int *values = malloc(sizeof(int) * NUM_ITEMS);
    if (values == NULL){
        tt_destroy_transposition_table(table);
        printf("malloc() failed\n");
        return 1;
    }
    start = clock();
    for (int i = 0; i < NUM_ITEMS; i++){
        values[i] = *((int *)tt_get_item(table, keys[i]));
    }
    end = clock();
    printf("time to retrieve %d entries: %f seconds\n", NUM_ITEMS, ((double) (end - start)) / CLOCKS_PER_SEC);

    // validate items retrieved
    for (int i = 0; i < NUM_ITEMS; i++){
        if (values[i] != i){
            printf("error: %d != %d\n", values[i], i);
            break;
        }
    }

    free(keys);
    free(values);
    tt_destroy_transposition_table(table);
    return 0;
}
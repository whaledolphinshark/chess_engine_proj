#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/transposition_table.h"

#define NUM_ITEMS 100000

uint64_t rand_uint64(){
    int int_bits = sizeof(int) * 8;
    int calls_needed = 64 / int_bits;

    uint64_t result = 0;
    for (int i = 0; i < calls_needed; i++){
        result |= ((uint64_t) rand()) << (i * int_bits);
    }

    return result;
}

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
    for (int i = 0; i < NUM_ITEMS; i++){
        keys[i] = rand_uint64();
    }
    clock_t start = clock();
    for (int i = 0; i < NUM_ITEMS; i++){
        tt_insert_item(table, keys[i], &i);
    }
    clock_t end = clock();
    printf("time to insert %d entries: %f seconds\n", NUM_ITEMS, ((double) (end - start)) / CLOCKS_PER_SEC);

    // check number of entries
    if (tt_get_num_items(table) != NUM_ITEMS){
        printf("number of items in table does not equal number of items inserted: %d != %d", tt_get_num_items(table), NUM_ITEMS);
    }

    // time to retrieve items
    int *values = malloc(sizeof(int) * NUM_ITEMS);
    if (values == NULL){
        free(keys);
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
            printf("item not retrieved: %d != %d\n", values[i], i);
            break;
        }
    }

    // time to check keys
    int *checks = malloc(sizeof(int) * NUM_ITEMS);
    if (checks == NULL){
        free(keys);
        free(values);
        tt_destroy_transposition_table(table);
        return 1;
    }
    start = clock();
    for (int i = 0; i < NUM_ITEMS; i++){
        checks[i] = tt_is_key_in_table(table, keys[i]);
    }
    end = clock();
    printf("time to check %d entries: %f seconds\n", NUM_ITEMS, ((double) (end - start)) / CLOCKS_PER_SEC);

    // validate checks
    for (int i = 0; i < NUM_ITEMS; i++){
        if (checks[i] != 1){
            printf("key not found in table: %lu\n", keys[i]);
        }
    }

    // time remove operations
    int *deletes = malloc(sizeof(int) * NUM_ITEMS);
    if (deletes == NULL){
        free(keys);
        free(values);
        free(checks);
        tt_destroy_transposition_table(table);
        return 1;
    }
    start = clock();
    for (int i = 0; i < NUM_ITEMS; i++){
        deletes[i] = tt_delete_item(table, keys[i]);
    }
    end = clock();
    printf("time to delete %d entries: %f seconds\n", NUM_ITEMS, ((double) (end - start)) / CLOCKS_PER_SEC);

    // validate deletes
    for (int i = 0; i < NUM_ITEMS; i++){
        if (deletes[i] != 1){
            printf("entry not deleted: %lu\n", keys[i]);
        }
    }

    free(keys);
    free(values);
    free(checks);
    free(deletes);
    tt_destroy_transposition_table(table);
    return 0;
}
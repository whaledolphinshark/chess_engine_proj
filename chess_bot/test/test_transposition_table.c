#include <stdio.h>
#include <stdlib.h>
#include <bits/getopt_core.h>
#include <time.h>

#include "chess_engine/chess_types.h"
#include "chess_engine/transposition_table.h"

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
    int opt;
    char *arg_s = NULL;
    char *arg_n = NULL;
    unsigned int seed = 1;
    unsigned int num_items = 100000;
    while ((opt = getopt(argc, argv, "s:n:")) != -1){
        switch (opt){
            case 's':
                arg_s = optarg;
                break;
            case 'n':
                arg_n = optarg;
                break;
            case '?':
            default:
                fprintf(stderr, "Error: unexpected argument '%s'\n", argv[optind]);
                fprintf(stderr, "Usage: tt_test [-s] [-n]\n-s: positive integer that is the seed the program should use\n-n: number of items the program should use\n");
                return 1;
        }
    }
    if (optind < argc) {
        fprintf(stderr, "Error: unexpected argument '%s'\n", argv[optind]);
        fprintf(stderr, "Usage: tt_test [-s] [-n]\n-s: positive integer that is the seed the program should use\n-n: number of items the program should use\n");
        return 1;
    }
    if (arg_s != NULL){
        char *end_ptr;
        unsigned long s = strtoul(arg_s, &end_ptr, 10);
        if (*end_ptr != '\0' || *arg_s == '-'){
            fprintf(stderr, "Error: invalid argument '%s'\n", arg_s);
            fprintf(stderr, "Usage: tt_test [-s] [-n]\n-s: positive integer that is the seed the program should use\n-n: number of items the program should use\n");
            return 1;
        }
        seed = (unsigned int)s;
    }
    if (arg_n != NULL){
        char *end_ptr;
        unsigned long n = strtoul(arg_n, &end_ptr, 10);
        if (*end_ptr != '\0' || *arg_n == '-'){
            fprintf(stderr, "Error: invalid argument '%s'\n", arg_n);
            fprintf(stderr, "Usage: tt_test [-s] [-n]\n-s: positive integer that is the seed the program should use\n-n: number of items the program should use\n");
            return 1;
        }
        num_items = (unsigned int)n;
    }
    srand(seed);

    _transposition_table *table = tt_create_transposition_table(sizeof(int));

    // time to store items
    uint64_t *keys = malloc(sizeof(uint64_t) * num_items);
    if (keys == NULL){
        tt_destroy_transposition_table(table);
        printf("malloc() failed\n");
        return 1;
    }
    for (int i = 0; i < num_items; i++){
        keys[i] = rand_uint64();
    }
    clock_t start = clock();
    for (int i = 0; i < num_items; i++){
        tt_insert_item(table, keys[i], &i);
    }
    clock_t end = clock();
    printf("time to insert %d entries: %f seconds\n", num_items, ((double) (end - start)) / CLOCKS_PER_SEC);

    // check number of entries
    if (tt_get_num_items(table) != num_items){
        printf("number of items in table does not equal number of items inserted: %d != %d", tt_get_num_items(table), num_items);
    }

    // time to retrieve items
    int *values = malloc(sizeof(int) * num_items);
    if (values == NULL){
        free(keys);
        tt_destroy_transposition_table(table);
        printf("malloc() failed\n");
        return 1;
    }
    start = clock();
    for (int i = 0; i < num_items; i++){
        values[i] = *((int *)tt_get_item(table, keys[i]));
    }
    end = clock();
    printf("time to retrieve %d entries: %f seconds\n", num_items, ((double) (end - start)) / CLOCKS_PER_SEC);

    // validate items retrieved
    for (int i = 0; i < num_items; i++){
        if (values[i] != i){
            printf("item not retrieved: %d != %d\n", values[i], i);
            break;
        }
    }

    // time to check keys
    int *checks = malloc(sizeof(int) * num_items);
    if (checks == NULL){
        free(keys);
        free(values);
        tt_destroy_transposition_table(table);
        return 1;
    }
    start = clock();
    for (int i = 0; i < num_items; i++){
        checks[i] = tt_is_key_in_table(table, keys[i]);
    }
    end = clock();
    printf("time to check %d entries: %f seconds\n", num_items, ((double) (end - start)) / CLOCKS_PER_SEC);

    // validate checks
    for (int i = 0; i < num_items; i++){
        if (checks[i] != 1){
            printf("key not found in table: %lu\n", keys[i]);
        }
    }

    // time remove operations
    int *deletes = malloc(sizeof(int) * num_items);
    if (deletes == NULL){
        free(keys);
        free(values);
        free(checks);
        tt_destroy_transposition_table(table);
        return 1;
    }
    start = clock();
    for (int i = 0; i < num_items; i++){
        deletes[i] = tt_delete_item(table, keys[i]);
    }
    end = clock();
    printf("time to delete %d entries: %f seconds\n", num_items, ((double) (end - start)) / CLOCKS_PER_SEC);

    // validate deletes
    for (int i = 0; i < num_items; i++){
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
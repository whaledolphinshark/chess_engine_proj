#ifndef     TRANSPOSITION_TABLE_INTERNAL_H
#define     TRANSPOSITION_TABLE_INTERNAL_H

#include <stdint.h>

#include "chess_engine/transposition_table.h"

#define MIN_NUM_BUCKETS 32
#define LOAD_FACTOR 2

extern uint64_t keys[64][12];
extern uint64_t black_turn_key;
extern uint64_t castling_keys[16];
extern uint64_t en_passant_keys[8];

typedef enum{
    FILLED = 0,
    EMPTY = 1
}_bucket_occupancy;

typedef struct _key_value_pair{
    _bucket_occupancy occupancy;
    uint64_t hash;
    struct _key_value_pair *next;
    void *value;
}_key_value_pair;

typedef struct _transposition_table{
    unsigned long item_size;
    int num_items;
    uint64_t num_buckets;
    _key_value_pair *buckets;
}_transposition_table;

#endif
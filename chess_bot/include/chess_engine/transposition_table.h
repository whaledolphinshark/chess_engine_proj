#ifndef     TRANSPOSITION_TABLE_H
#define     TRANSPOSITION_TABLE_H

#include "chess_types.h"

_transposition_table *tt_create_transposition_table(unsigned long type_size);
void tt_insert_item(_transposition_table *hash_table, uint64_t zobrist_hash, void *item);
void *tt_get_item(_transposition_table *hash_table, uint64_t zobrist_hash);
int tt_delete_item(_transposition_table *hash_table, uint64_t zobrist_hash);
int tt_is_key_in_table(_transposition_table *hash_table, uint64_t zobrist_hash);
void tt_clear_items(_transposition_table *hash_table);
int tt_get_num_items(_transposition_table *hash_table);
void tt_destroy_transposition_table(_transposition_table *hash_table);

#endif
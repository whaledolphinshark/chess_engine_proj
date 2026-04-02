#include <stdlib.h>
#include <string.h>

#include "chess_engine/transposition_table.h"
#include "utils/bitboard_util.h"
#include "transposition_table_internal.h"
#include "utils/error_handling.h"

static _key_value_pair *malloc_buckets(uint64_t num_buckets){
    _key_value_pair *new_buckets = malloc(sizeof(_key_value_pair) * num_buckets);
    if (new_buckets == NULL){
        eh_die("malloc() failed");
    }

    for (int i = 0; i < num_buckets; i++){
        _key_value_pair empty_bucket = {EMPTY, 0, NULL, NULL};
        new_buckets[i] = empty_bucket;
    }

    return new_buckets;
}

// 1 if found open spot in which case value of entry are copied and entry needs to be freed if malloced
// 0 if entry is placed in a linked list and does not need to be freed
// -1 if entry was already there and its value was replaced
static int place_entry(_key_value_pair *buckets, _key_value_pair *entry, uint64_t index){
    _key_value_pair *bucket = &(buckets[index]);
    if (bucket->occupancy == EMPTY){
        bucket->occupancy = FILLED;
        bucket->hash = entry->hash;
        bucket->next = NULL;
        bucket->value = entry->value;
        return 1;
    }
    else if(bucket->hash == entry->hash){
        free(bucket->value);
        bucket->value = entry->value;
        return -1;
    }
    else{
        while (bucket->next != NULL){
            bucket = bucket->next;
            if (bucket->hash == entry->hash){
                free(bucket->value);
                bucket->value = entry->value;
                return -1;
            }
        }
        bucket->next = entry;
    }
    return 0;
}

static void resize_table(_transposition_table *hash_table, int expand){
    uint64_t new_num_buckets;
    if (expand == 1){
        new_num_buckets = hash_table->num_buckets << 2;
    }
    else{
        new_num_buckets = hash_table->num_buckets >> 2;
    }
    _key_value_pair *new_buckets = malloc_buckets(new_num_buckets);

    for (int i = 0; i < hash_table->num_buckets; i++){
        _key_value_pair *first_entry = &(hash_table->buckets[i]);
        if (first_entry->occupancy == EMPTY){
            continue;
        }
        _key_value_pair *next = first_entry->next;
        
        // the first bucket in the linked list is not malloced since the entire region it resides in is malloced instead
        // kinda weird how i did it but oh well
        _key_value_pair *bucket = malloc(sizeof(_key_value_pair));
        if (bucket == NULL){
            eh_die("malloc() failed");
        }
        bucket->hash = first_entry->hash;
        bucket->occupancy = FILLED;
        bucket->value = first_entry->value;
        bucket->next = NULL;
        
        int code = place_entry(new_buckets, bucket, bucket->hash & (new_num_buckets - 1));
        if (code == 1){
            free(bucket);
        }

        while (next != NULL){
            bucket = next;
            next = next->next;
            bucket->next = NULL;

            int code = place_entry(new_buckets, bucket, bucket->hash & (new_num_buckets - 1));
            if (code == 1){
                free(bucket);
            }
        }
    } 

    free(hash_table->buckets);
    hash_table->buckets = new_buckets;
    hash_table->num_buckets = new_num_buckets;
}

_transposition_table *tt_create_transposition_table(unsigned long type_size){
    _transposition_table *hash_table = (_transposition_table *)malloc(sizeof(_transposition_table));
    if (hash_table == NULL){
        eh_die("malloc() failed");
    }

    hash_table->item_size = type_size;
    hash_table->num_buckets = MIN_NUM_BUCKETS;
    hash_table->num_items = 0;
    // buckets is an array of lists
    hash_table->buckets = malloc_buckets(MIN_NUM_BUCKETS);

    return hash_table;
}

void tt_insert_item(_transposition_table *hash_table, uint64_t zobrist_hash, void *item){
    if (hash_table == NULL){
        eh_die("passed in null pointers");
    }

    // check if need to expand
    if (hash_table->num_items >= hash_table->num_buckets * LOAD_FACTOR){
        resize_table(hash_table, 1);
    }

    void *item_ptr = malloc(hash_table->item_size);
    _key_value_pair *entry = (_key_value_pair *)malloc(sizeof(_key_value_pair));
    if (item_ptr == NULL || entry == NULL){
        eh_die("malloc() failed");
    }
    memcpy(item_ptr, item, hash_table->item_size);

    entry->occupancy = FILLED;
    entry->hash = zobrist_hash;
    entry->next = NULL;
    entry->value = item_ptr;

    int code = place_entry(hash_table->buckets, entry, entry->hash & (hash_table->num_buckets - 1));
    if (code == 1 || code == -1){
        free(entry);
    }

    if (code != -1){
        hash_table->num_items++;
    }
}

void *tt_get_item(_transposition_table *hash_table, uint64_t zobrist_hash){
    if (hash_table == NULL){
        eh_die("passed in null pointers");
    }

    uint64_t index = zobrist_hash & (hash_table->num_buckets - 1);
    _key_value_pair *bucket = &(hash_table->buckets[index]);
    if (bucket->occupancy == EMPTY){
        eh_die("item not found");
        exit(EXIT_FAILURE);
    }
    
    while (bucket != NULL){
        if (bucket->hash == zobrist_hash){
            return bucket->value;
        }

        bucket = bucket->next;
    }

    eh_die("item not found");
    exit(EXIT_FAILURE);
}

// return 1 if found, 0 if not
int tt_delete_item(_transposition_table *hash_table, uint64_t zobrist_hash){
    if (hash_table == NULL){
        eh_die("passed in null pointers");
    }

    uint64_t index = zobrist_hash & (hash_table->num_buckets - 1);
    _key_value_pair *bucket = &(hash_table->buckets[index]);

    if (bucket->occupancy == EMPTY){
        return 0;
    }
    else if (bucket->hash == zobrist_hash){
        hash_table->num_items--;
        _key_value_pair *next = bucket->next;
        if (next == NULL){
            bucket->occupancy = EMPTY;
            free(bucket->value);
            if (hash_table->num_items * 2 * LOAD_FACTOR < hash_table->num_buckets && hash_table->num_buckets > MIN_NUM_BUCKETS){
                resize_table(hash_table, 0);
            }
            return 1;
        }
        
        free(bucket->value);
        bucket->hash = next->hash;
        bucket->value = next->value;
        bucket->next = next->next;
        free(next);
        if (hash_table->num_items * 2 * LOAD_FACTOR < hash_table->num_buckets && hash_table->num_buckets > MIN_NUM_BUCKETS){
            resize_table(hash_table, 0);
        }
        return 1;
    }

    while (bucket->next != NULL){
        if (bucket->next->hash == zobrist_hash){
            hash_table->num_items--;
            _key_value_pair *next = bucket->next;
            bucket->next = next->next;
            free(next->value);
            free(next);
            if (hash_table->num_items * 2 * LOAD_FACTOR < hash_table->num_buckets && hash_table->num_buckets > MIN_NUM_BUCKETS){
                resize_table(hash_table, 0);
            }
            return 1;
        }

        bucket = bucket->next;
    }

    return 0;
}

int tt_is_key_in_table(_transposition_table *hash_table, uint64_t zobrist_hash){
    if (hash_table == NULL){
        eh_die("passed in null pointers");
    }
    
    uint64_t index = zobrist_hash & (hash_table->num_buckets - 1);
    _key_value_pair *bucket = &(hash_table->buckets[index]);
    if (bucket->occupancy == EMPTY){
        return 0;
    }

    while (bucket != NULL){
        if (bucket->hash == zobrist_hash){
            return 1;
        }

        bucket = bucket->next;
    }

    return 0;
}

void tt_clear_items(_transposition_table *hash_table){
    if (hash_table == NULL){
        eh_die("passed in null pointer");
    }

    _key_value_pair *new_buckets = malloc_buckets(MIN_NUM_BUCKETS);

    for (int i = 0; i < hash_table->num_buckets; i++){
        // no need to free the first entry since that is actually not malloced
        _key_value_pair *bucket = &(hash_table->buckets[i]);
        if (bucket->occupancy == EMPTY){
            continue;
        }
        _key_value_pair *next = bucket->next;

        free(bucket->value);
        bucket->occupancy = EMPTY;
        bucket->next = NULL;
        while (next != NULL){
            free(next->value);
            bucket = next;
            next = next->next;
            free(bucket);
        }
    }

    free(hash_table->buckets);
    hash_table->buckets = new_buckets;
    hash_table->num_buckets = MIN_NUM_BUCKETS;
    hash_table->num_items = 0;
}

int tt_get_num_items(_transposition_table *hash_table){
    if (hash_table == NULL){
        eh_die("passed in null pointer");
    }

    return hash_table->num_items;
}

unsigned long tt_get_item_size(_transposition_table *hash_table){
    return hash_table->item_size;
}

void tt_destroy_transposition_table(_transposition_table *hash_table){
    if (hash_table == NULL){
        eh_die("passed in null pointer");
    }

    tt_clear_items(hash_table);
    free(hash_table->buckets);
    free(hash_table);
}
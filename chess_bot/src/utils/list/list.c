#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/list.h"
#include "list_internal.h"
#include "utils/error_handling.h"

_list *gl_create_list(unsigned long type_size){

    _list *list = malloc(sizeof(_list));
    if (list == NULL){
        eh_die("malloc() failed");
    }

    list->capacity = MIN_LIST_CAPACITY;
    list->base = malloc(type_size * MIN_LIST_CAPACITY);
    if (list->base == NULL){
        gl_destroy_list(list);
        eh_die("malloc() failed");
    }

    list->length = 0;
    list->type_size = type_size;

    return list;
}

void gl_append_item(_list *list, void *item){
    if (list == NULL){
        eh_die("passed in null pointer");
    }

    if (list->length + 1 > list->capacity){
        list->base = realloc(list->base, 2 * list->capacity * list->type_size);
        if (list->base == NULL){
            eh_die("malloc() failed");
        }

        list->capacity *= 2;
    }

    void *target = (char *)list->base + list->length * list->type_size;
    memcpy(target, item, list->type_size);

    list->length++;
}

void *gl_access_item(_list *list, unsigned long index){
    if (list == NULL){
        eh_die("passed in null pointer");
    }
    if (index >= list->length){
        eh_die("index out of bounds");
    }

    return (char *)list->base + index * list->type_size;
}

void gl_set_item(_list *list, void *item, unsigned long index){
    if (list == NULL){
        eh_die("passed in null pointer");
    }
    if (index >= list->length){
        eh_die("index out of bounds");
    }

    void *target = (char *)list->base + index * list->type_size;
    memcpy(target, item, list->type_size);
}

void gl_insert_item(_list *list, void *item, unsigned long index){
    if (list == NULL){
        eh_die("passed in null pointer");
    }
    if (index > list->length){
        eh_die("index out of bounds");
    }

    if (index == list->length){
        gl_append_item(list, item);
        return;
    }

    if (list->length + 1 > list->capacity){
        list->base = realloc(list->base, 2 * list->capacity * list->type_size);
        if (list->base == NULL){
            eh_die("malloc() failed");
        }

        list->capacity *= 2;
    }

    void *target = (char *)list->base + index * list->type_size;
    memmove((char *)target + list->type_size, target, (list->length - index - 1) * list->type_size);
    memcpy(target, item, list->type_size);

    list->length++;
}

void gl_remove_item(_list *list, unsigned long index){
    if (list == NULL){
        eh_die("passed in null pointer");
    }
    if (index >= list->length){
        eh_die("index out of bounds");
    }

    if (index != list->length - 1){
        void *target = (char *)list->base + index * list->type_size;
        memmove(target, (char *)target + list->type_size, (list->length - index - 1) * list->type_size);
    }

    list->length--;

    if (list->capacity > MIN_LIST_CAPACITY && 2 * (list->length + 1) < list->capacity){
        list->base = realloc(list->base, list->capacity * list->type_size / 2);
        list->capacity /= 2;
    }
}

void gl_extract_item(_list *list, unsigned long index, void *buffer){
    if (list == NULL || buffer == NULL){
        eh_die("passed in null pointer");
    }
    if (index >= list->length){
        eh_die("index out of bounds");
    }

    void *target = (char *)list->base + index * list->type_size;
    memcpy(buffer, target, list->type_size);

    if (index != list->length - 1){
        memmove(target, (char *)target + list->type_size, (list->length - index - 1) * list->type_size);
    }

    list->length--;

    if (list->capacity > MIN_LIST_CAPACITY && 2 * (list->length + 1) < list->capacity){
        list->base = realloc(list->base, list->capacity * list->type_size / 2);
        list->capacity /= 2;
    }
}

void gl_clear_list(_list *list){
    if (list == NULL){
        eh_die("passed in null pointer");
    }
    list->base = realloc(list->base, MIN_LIST_CAPACITY * list->type_size);
    list->length = 0;
    list->capacity = MIN_LIST_CAPACITY;
}

unsigned long gl_get_length(_list *list){
    if (list == NULL){
        eh_die("passed in null pointer");
    }
    return list->length;
}

void gl_destroy_list(_list *list){
    if (list == NULL){
        eh_die("passed in null pointer");
    }

    if (list->base != NULL){
        free(list->base);
    }
    free(list);
}

unsigned long gl_size_of_list(){
    return sizeof(_list);
}

unsigned long gl_get_type_size(_list *list){
    return list->type_size;
}
#ifndef     LIST_INTERNAL_H
#define     LIST_INTERNAL_H

#define MIN_LIST_CAPACITY 8

typedef struct _list{
    void *base;
    unsigned long type_size;
    unsigned long capacity;
    unsigned long length;
} _list;

#endif
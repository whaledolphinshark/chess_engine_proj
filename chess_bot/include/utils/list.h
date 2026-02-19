#ifndef     LIST_H
#define     LIST_H

typedef struct _list _list;

_list *gl_create_list(unsigned long type_size);
void gl_append_item(_list *list, void *item);
void *gl_access_item(_list *list, unsigned long index);
void gl_set_item(_list *list, void *item, unsigned long index);
void gl_insert_item(_list *list, void *item, unsigned long index);
void gl_remove_item(_list *list, unsigned long index);
void gl_extract_item(_list *list, unsigned long index, void *buffer);
void gl_clear_list(_list *list);
unsigned long gl_get_length(_list *list);
void gl_destroy_list(_list *list);
unsigned long gl_size_of_list();
unsigned long gl_get_type_size(_list *list);

#endif
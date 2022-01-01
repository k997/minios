#ifndef __LIB_LIST_H
#define __LIB_LIST_H

#include "stdint.h"

// 结构体成员的相对于结构体地址偏移
#define offsetof(struct_type, struct_member) (int)(&(((struct_type *)0)->struct_member))
// 通过结构体成员获取结构体地址
#define container_of(struct_type, struct_member, struct_member_ptr) \
    (struct_type *)((int)struct_member_ptr - offsetof(struct_type, struct_member))

typedef struct list_elem
{
    struct list_elem *prev;
    struct list_elem *next;
} list_elem;
typedef struct list
{
    struct list_elem head;
    struct list_elem tail;
} list;

typedef bool(function)(list_elem *, int arg);

void list_init(list *list);
void list_insert_before(list_elem *elem, list_elem *new);
void list_push(list *plist, list_elem *new);
void list_append(list *plist, list_elem *new);
void list_remove(list_elem *elem);
list_elem *list_pop(list *plist);
bool list_find_elem(list *plist, list_elem *obj_elem);
list_elem *list_traversal(list *plist, function func, int arg);
uint32_t list_len(list *plist);
bool list_empty(list *plist);

#endif
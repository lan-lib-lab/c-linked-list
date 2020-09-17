#ifndef LIB_H
#define LIB_H

#include <stdbool.h>
#include "mini_inttypes.h"

typedef struct {
    struct ll_LinkedListNode *head;
    struct ll_LinkedListNode *tail;
    u32 data_size;
    u32 len;
}ll_LinkedList;

struct ll_LinkedListNode {
    struct ll_LinkedListNode *next;
    struct ll_LinkedListNode *prev;
    u8 data[];
};


// different linkedlist implementation that does not use malloc
typedef struct {
    struct ll_BufferLinkedListNode *head;
    struct ll_BufferLinkedListNode *tail;
    void *buffer;
    const u32 data_size;
}ll_BufferLinkedList;

struct ll_BufferLinkedListNode {
    struct ll_BufferLinkedListNode *next;
    u8 data[];
};

typedef enum {
    LL_OK,
    LL_ERROR_NULL_LINKED_LIST_POINTER,
    LL_ERROR_NULL_ELEMENT_POINTER,
    LL_ERROR_NULL_NODE_POINTER,
    LL_ERROR_EMPTY_LINKED_LIST,
    LL_ERROR_INDEX_OUT_OF_BOUNDS,
    LL_ERROR_INIT_FAILURE,
    LL_ERROR_MALLOC_FAILURE,
    LL_ERROR_INSUFFICIENT_SIZE,
    LL_ERROR_INTERNAL,
}ll_Error;

ll_LinkedList* ll_new(u32 data_size);
ll_Error ll_free(ll_LinkedList **self);
bool ll_is_empty(const ll_LinkedList *self);
ll_Error ll_push(ll_LinkedList *self, void *elem);
ll_Error ll_push_front(ll_LinkedList *self, void *elem);
ll_Error ll_pop(ll_LinkedList *self, void *out_elem);
ll_Error ll_pop_front(ll_LinkedList *self, void *out_elem);
ll_Error iterate_to(const ll_LinkedList *self, struct ll_LinkedListNode **out_node, int index);
ll_Error ll_insert(ll_LinkedList *self, int index, void *elem);
ll_Error ll_get(const ll_LinkedList *self, void *out_elem, int index);
ll_Error ll_remove(ll_LinkedList *self, void *out_elem, int index);


ll_Error ll_buf_init(ll_BufferLinkedList *self, void *buffer, int data_size);
ll_Error ll_buf_push(ll_BufferLinkedList *self, void *elem);
ll_Error ll_buf_push_front(ll_BufferLinkedList *self, void *elem);
ll_Error ll_buf_pop(ll_BufferLinkedList *self);
ll_Error ll_buf_pop_front(ll_BufferLinkedList *self);
ll_Error ll_buf_insert(ll_BufferLinkedList *self, int index, void *elem);
ll_Error ll_buf_get(const ll_BufferLinkedList *self, int index, void **out);
ll_Error ll_buf_remove(ll_BufferLinkedList *self, int index);

#endif // LIB_H

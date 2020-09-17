#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "lib.h"
#define LOG_DEBUG_ENABLE
/* #define ERROR_RETURN_LOG_DISABLE */
#include "common_macros.h"


/// returns NULL on failure
ll_LinkedList* ll_new(u32 data_size) {
    ll_LinkedList *out = (ll_LinkedList*)malloc(sizeof(ll_LinkedList));
    if (out == NULL) return NULL;
    out->head = NULL;
    out->tail = NULL;
    out->data_size = data_size;
    out->len = 0;
    return out;
}


/// deallocates the linkedlist and all of the nodes in it
/// also sets the pointer to NULL to detect double free
/// returns LL_OK 
///     || LL_ERROR_NULL_LINKED_LIST_POINTER (can happen with double free)
ll_Error ll_free(ll_LinkedList **self) {
    if (*self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;

    struct ll_LinkedListNode *node = (*self)->head;
    while (node) {
        struct ll_LinkedListNode *next = node->next;
        free(node);
        node = next;
    }

    free(*self);
    // set to NULL to prevent this now invalidated pointer from being used
    *self = NULL;

    return LL_OK;
}


inline bool ll_is_empty(const ll_LinkedList *self) {
    return (self != NULL 
            && self->head == NULL && self->tail == NULL
            && self->len == 0);
}

// checks that the head and the tail are always at the edges of the linked list if they are initialized
// except when there is only one element -- then the head and tail must point to the same spot.
static inline ll_Error has_valid_head_tail_state(const ll_LinkedList *self, bool *res) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (self->len < 0) return LL_ERROR_INTERNAL;

    bool out = true;
    // if there's only one node, ensure both #self->head and #self->tail point to it
    if (self->len == 1 && (self->head != self->tail || self->head == NULL)) out = false;

    // ensure NULL prev and next nodes for #self->head and #self->tail
    if (self->len != 1 && self->head != NULL && self->head->prev != NULL) out = false;
    if (self->len != 1 && self->tail != NULL && self->tail->next != NULL) out = false;

    *res = out;
    return LL_OK;
}

/// checks that the given index is within the valid range for [0..self->len].
static inline ll_Error is_index_within_get_bounds(const ll_LinkedList *self, bool *res, int index) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (self->len < 0) return LL_ERROR_INTERNAL;

    bool out = true;
    if (index < 0 || index >= (int)self->len) out = false;
    
    *res = out;
    return LL_OK;
}

/// checks that the given index is within the valid insertion range for [0..=self->len]
/// if the index is self->len, this indicates a push to tail.
static inline ll_Error is_index_within_insert_bounds(const ll_LinkedList *self, bool *res, int index) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (self->len < 0) return LL_ERROR_INTERNAL;

    bool out = true;
    if (index < 0 || index > (int)self->len) out = false;

    *res = out;
    return LL_OK;
}

/// pushes an element node to the tail of the linked list.
/// if it's the first item, the head and tail point to it.
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_NULL_ELEMENT_POINTER
///     || LL_ERROR_MALLOC_FAILURE
///     || LL_ERROR_INTERNAL
ll_Error ll_push(ll_LinkedList *self, void *elem) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (elem == NULL) return LL_ERROR_NULL_ELEMENT_POINTER;
    if (!EXPECT_S(has_valid_head_tail_state, self, bool)) ERROR_RETURN(LL_ERROR_INTERNAL);

    if (self->len < 0) ERROR_RETURN(LL_ERROR_INTERNAL);
    if (ll_is_empty(self)) {
        if (self->head != NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
        self->head = malloc(sizeof(struct ll_LinkedListNode));
        if (self->head == NULL) ERROR_RETURN(LL_ERROR_MALLOC_FAILURE);

        self->head->prev = NULL;
        self->head->next = NULL;
        memcpy(self->head->data, elem, self->data_size);
        self->tail = self->head;
    } else {
        if (self->head == NULL || self->tail == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);

        self->tail->next = malloc(sizeof(struct ll_LinkedListNode));
        if (self->tail->next == NULL) ERROR_RETURN(LL_ERROR_MALLOC_FAILURE);
        self->tail->next->prev = self->tail;
        self->tail->next->next = NULL;
        
        memcpy(self->tail->next->data, elem, self->data_size);
        
        self->tail = self->tail->next;
    }

    self->len++;
    return LL_OK;
}


/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_NULL_ELEMENT_POINTER
///     || LL_ERROR_MALLOC_FAILURE
///     || LL_ERROR_INTERNAL
ll_Error ll_push_front(ll_LinkedList *self, void *elem) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (elem == NULL) return LL_ERROR_NULL_ELEMENT_POINTER;

    if (ll_is_empty(self)) {
        EXPECT_PASS(ll_push(self, elem));
    } else {
        if (!EXPECT_S(has_valid_head_tail_state, self, bool)) ERROR_RETURN(LL_ERROR_INTERNAL);

        self->head->prev = malloc(sizeof(struct ll_LinkedListNode));
        if (self->head->prev == NULL) ERROR_RETURN(LL_ERROR_MALLOC_FAILURE);
        self->head->prev->prev = NULL;
        self->head->prev->next = self->head;
        memcpy(self->head->prev->data, elem, self->data_size);
        
        self->head = self->head->prev;
    
        if (self->len < 0) ERROR_RETURN(LL_ERROR_INTERNAL);
        self->len++;
    }

    return LL_OK;
}


/// param out_elem: node element is written to it, or ignored if NULL.
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_EMPTY_LINKED_LIST
///     || LL_ERROR_INTERNAL
ll_Error ll_pop(ll_LinkedList *self, void *out_elem) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (ll_is_empty(self)) return LL_ERROR_EMPTY_LINKED_LIST;
    if (self->head == NULL || self->tail == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
    if (!EXPECT_S(has_valid_head_tail_state, self, bool)) ERROR_RETURN(LL_ERROR_INTERNAL);
    
    // copy node data to out_elem if not NULL
    struct ll_LinkedListNode *node = self->tail;
    if (out_elem != NULL) memcpy(out_elem, self->tail->data, self->data_size);

    if (self->head == self->tail) {
        // empty after a pop
        self->head = NULL;
        self->tail = NULL;
    } else {
        if (self->tail->prev == self->tail) ERROR_RETURN(LL_ERROR_INTERNAL);
        self->tail = self->tail->prev;
        self->tail->next = NULL;
    }

    if (self->len <= 0) ERROR_RETURN(LL_ERROR_INTERNAL);
    self->len--;

    free(node);
    return LL_OK;
}


/// @param out_elem  node element is written to it, or ignored if NULL.
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_EMPTY_LINKED_LIST
///     || LL_ERROR_INTERNAL
ll_Error ll_pop_front(ll_LinkedList *self, void *out_elem) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (ll_is_empty(self)) return LL_ERROR_EMPTY_LINKED_LIST;
    
    // copy node data to out_elem if not NULL
    struct ll_LinkedListNode *node = self->head;
    if (out_elem != NULL) memcpy(out_elem, node->data, self->data_size);

    if (self->head == self->tail) {
        // empty after a pop
        self->head = NULL;
        self->tail = NULL;
    } else {
        if (self->head == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
        self->head = self->head->next;
        self->head->prev = NULL;
    }
    
    if (self->len <= 0) ERROR_RETURN(LL_ERROR_INTERNAL);
    self->len--;

    free(node);
    return LL_OK;
}


/// determines the shortest path from head to index or tail to index and iterates through it
/// the node at the index is written to out_node
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_NULL_NODE_POINTER
///     || LL_ERROR_EMPTY_LINKED_LIST
///     || LL_ERROR_INDEX_OUT_OF_BOUNDS
///     || LL_ERROR_INTERNAL
ll_Error iterate_to(const ll_LinkedList *self, struct ll_LinkedListNode **out_node, int index) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (out_node == NULL) return LL_ERROR_NULL_NODE_POINTER;
    if (ll_is_empty(self)) return LL_ERROR_EMPTY_LINKED_LIST;
    if (!EXPECT_S(is_index_within_get_bounds, self, bool, index)) return LL_ERROR_INDEX_OUT_OF_BOUNDS;
    if (!EXPECT_S(has_valid_head_tail_state, self, bool)) ERROR_RETURN(LL_ERROR_INTERNAL);

    // iterate to target node to retrieve
    struct ll_LinkedListNode *target = NULL;
    if ((int)self->len - index < index) {
        // better iterate in reverse since the distance from tail is shorter
        for (target=self->tail; target != NULL; target = target->prev) {
            if (self->len - ++index == 0) break;
        }
    } else {
        // iterate from head
        for (target=self->head; target != NULL; target = target->next) {
            if (index-- == 0) break;
        }
    }
    
    if (target == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
    *out_node = target;
    return LL_OK;
}


/// traverses to the nth node (unless it's tail or head) and left-inserts a node with the element content at
/// the given index.
/// @param index must be within the raneg [0..self->len()] where self->len() indicates a tail insert.
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_NULL_ELEMENT_POINTER
///     || LL_ERROR_INDEX_OUT_OF_BOUNDS
///     || LL_ERROR_MALLOC_FAILURE
///     || LL_ERROR_INTERNAL
ll_Error ll_insert(ll_LinkedList *self, int index, void *elem) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (elem == NULL) return LL_ERROR_NULL_ELEMENT_POINTER;
    if (!EXPECT_S(is_index_within_insert_bounds, self, bool, index)) return LL_ERROR_INDEX_OUT_OF_BOUNDS;
    
    if (self->len == 0 || index == (int)self->len) {
        // one element, or at the end. Both of those insertions are handled with a push to tail
        EXPECT_PASS(ll_push(self, elem));
    }
    else if (index == 0) {
        // insertion in the front, but this is not an empty linkedlist, that's a front push
        EXPECT_PASS(ll_push_front(self, elem));
    }
    else {
        // traverse to node to insert at
        struct ll_LinkedListNode *target_node = NULL; 
        ll_Error status = iterate_to(self, &target_node, index);
        if (status != LL_OK) ERROR_RETURN(LL_ERROR_INTERNAL);
        if (target_node == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);

        // insert new node to the left of #target_node at index
        struct ll_LinkedListNode *node = malloc(sizeof(struct ll_LinkedListNode));
        if (node == NULL) ERROR_RETURN(LL_ERROR_MALLOC_FAILURE);
        node->next = target_node;
        node->prev = target_node->prev;
        memcpy(node->data, elem, self->data_size);

        target_node->prev = node;
        self->len++;
    }

    return LL_OK;
}


/// traverses to the nth node (unless it's a tail or head) and sets the data of that node to the content of #elem.
/// @param index must be within the range [0..self->len())
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_NULL_ELEMENT_POINTER
///     || LL_ERROR_INDEX_OUT_OF_BOUNDS
///     || LL_ERROR_INTERNAL
ll_Error ll_set(ll_LinkedList *self, int index, void *elem) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (elem == NULL) return LL_ERROR_NULL_ELEMENT_POINTER;
    if (!EXPECT_S(is_index_within_get_bounds, self, bool, index)) return LL_ERROR_INDEX_OUT_OF_BOUNDS;
    if (!EXPECT_S(has_valid_head_tail_state, self, bool)) ERROR_RETURN(LL_ERROR_INTERNAL);

    // iterate to target node to retrieve (or error return)
    struct ll_LinkedListNode *target = EXPECT_S(iterate_to, self, struct ll_LinkedListNode*, index);

    // retrieve target node data
    if (target == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
    memcpy(target->data, elem, self->data_size);
    
    return LL_OK;
}

/// retrieves the data in a node at an index without removing that node.
/// it's preferable to iterate rather than use this function as it's O(n)
/// @param out_elem node data to be gotten.
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_NULL_ELEMENT_POINTER
///     || LL_ERROR_EMPTY_LINKED_LIST,
///     || LL_ERROR_INDEX_OUT_OF_BOUNDS
///     || LL_ERROR_INTERNAL
ll_Error ll_get(const ll_LinkedList *self, void *out_elem, int index) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (out_elem == NULL) return LL_ERROR_NULL_ELEMENT_POINTER;
    if (ll_is_empty(self)) return LL_ERROR_EMPTY_LINKED_LIST;
    if (!EXPECT_S(is_index_within_get_bounds, self, bool, index)) return LL_ERROR_INDEX_OUT_OF_BOUNDS;

    // iterate to target node to retrieve (or error return)
    struct ll_LinkedListNode *target = EXPECT_S(iterate_to, self, struct ll_LinkedListNode*, index);

    // retrieve target node data
    if (target == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
    memcpy(out_elem, target->data, self->data_size);
    
    return LL_OK;
}


/// @param out_elem data of removed element to be written. if NULL, it's discarded.
/// @param index must be [0..self->len) 
/// @returns LL_OK
///     || LL_ERROR_NULL_LINKED_LIST_POINTER
///     || LL_ERROR_EMPTY_LINKED_LIST
///     || LL_ERROR_INDEX_OUT_OF_BOUNDS
///     || LL_ERROR_INTERNAL
ll_Error ll_remove(ll_LinkedList *self, void *out_elem, int index) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (ll_is_empty(self)) return LL_ERROR_EMPTY_LINKED_LIST;
    if (!EXPECT_S(is_index_within_get_bounds, self, bool, index)) return LL_ERROR_INDEX_OUT_OF_BOUNDS;

    if (self->len == 1 || index == (int)self->len-1) {
        EXPECT_PASS(ll_pop(self, out_elem));
    }
    else if (index == 0) {
        EXPECT_PASS(ll_pop_front(self, out_elem));
    }
    else {
        struct ll_LinkedListNode *target = EXPECT_S(iterate_to, self, struct ll_LinkedListNode*, index);

        // link the target's prev with next as it must be a middle node
        if (target == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
        if (target->prev == NULL || target->next == NULL) ERROR_RETURN(LL_ERROR_INTERNAL);
        target->prev->next = target->next;
        target->next->prev = target->prev;

        if (out_elem != NULL) memcpy(out_elem, target->data, self->data_size);
        free(target);
        self->len--;
    }

    return LL_OK;
}



ll_Error ll_buf_init(ll_BufferLinkedList *self, void *buffer, int data_size);
ll_Error ll_buf_push(ll_BufferLinkedList *self, void *elem);
ll_Error ll_buf_pop(ll_BufferLinkedList *self);
ll_Error ll_buf_insert(ll_BufferLinkedList *self, int index, void *elem);
ll_Error ll_buf_get(const ll_BufferLinkedList *self, int index, void **out);
ll_Error ll_buf_remove(ll_BufferLinkedList *self, int index);



#ifndef CUNIT_TESTS
int main(void) {
    ll_LinkedList *list = ll_new(sizeof(int));
    int x = 12;
    int y;

    for (int i=0; true; i++) {
        int y;
        ll_push(list, &i);
        ll_pop(list, &y);

        sleep(1);
    }

    ll_push(list, &x);
    printf("list[0] = %d\n", ll_get(list, &y, 0));
    printf("Hello Tester!\n");
    return 0;
}
#endif


#ifdef CUNIT_TESTS
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
#include "cunit_utils/lib.h"

ll_LinkedList* assert_new(u32 data_size) {
    ll_LinkedList *ll = ll_new(data_size);
    CU_ASSERT_PTR_NULL(ll->head);
    CU_ASSERT_PTR_NULL(ll->tail);
    CU_ASSERT(ll->data_size == data_size);
    CU_ASSERT(ll->len == 0);
    return ll;
}

bool assert_push_u32(ll_LinkedList *ll, u32 val) {
    ll_Error status = ll_push(ll, (u32[]){val});
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(CUU_ASSERT_PTR_NOT_NULL(ll->tail));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(*((u32*)ll->tail->data), val));
    return true;
}

bool assert_pop_u32(ll_LinkedList *ll, u32 exp) {
    int out_val;
    ll_Error status = ll_pop(ll, &out_val);
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(out_val, exp));
    return true;
}

bool assert_pop_empty_u32(ll_LinkedList *ll) {
    int out_val;
    ll_Error status = ll_pop(ll, &out_val);
    EXPECT_TRUE(CUU_ASSERT(status == LL_ERROR_EMPTY_LINKED_LIST));
    return true;
}

bool assert_push_front_u32(ll_LinkedList *ll, u32 val) {
    ll_Error status = ll_push_front(ll, (u32[]){val});
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(CUU_ASSERT_PTR_NOT_NULL(ll->tail));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(*((u32*)ll->head->data), val));
    return true;
}

bool assert_pop_front_u32(ll_LinkedList *ll, u32 exp) {
    int out_val;
    ll_Error status = ll_pop_front(ll, &out_val);
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(out_val, exp));
    return true;
}

bool assert_pop_front_empty_u32(ll_LinkedList *ll) {
    int out_val;
    ll_Error status = ll_pop(ll, &out_val);
    EXPECT_TRUE(CUU_ASSERT(status == LL_ERROR_EMPTY_LINKED_LIST));
    return true;
}

bool assert_iterate_to_u32(ll_LinkedList *ll, int index, u32 exp) {
    struct ll_LinkedListNode *target;
    ll_Error status = iterate_to(ll, &target, index);
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(CUU_ASSERT_PTR_NOT_NULL(target));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(*(u32*)target->data, exp));
    return true;
}

bool assert_iterate_to_u32_error(ll_LinkedList *ll, int index, ll_Error exp) {
    struct ll_LinkedListNode *target;
    ll_Error status = iterate_to(ll, &target, index);
    EXPECT_TRUE(CUU_ASSERT(status == exp));
    return true;
}

bool assert_insert_u32(ll_LinkedList *ll, int index, u32 data) {
    int len = ll->len;
    ll_Error status = ll_insert(ll, index, &data);
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(assert_iterate_to_u32(ll, index, data));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(ll->len, len+1));
    return true;
}

bool assert_insert_u32_error(ll_LinkedList *ll, int index, u32 data, ll_Error exp) {
    ll_Error status = ll_insert(ll, index, &data);
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(status, exp));
    return true;
}

bool assert_set_u32(ll_LinkedList *ll, int index, u32 data) {
    ll_Error status = ll_set(ll, index, &data);
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(status, LL_OK));
    EXPECT_TRUE(assert_iterate_to_u32(ll, index, data));
    return true;
}

bool assert_set_u32_error(ll_LinkedList *ll, int index, u32 data, ll_Error exp) {
    ll_Error status = ll_set(ll, index, &data);
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(status, exp));
    return true;
}

bool assert_get_u32(ll_LinkedList *ll, int index, u32 exp) {
    u32 res;
    ll_Error status = ll_get(ll, &res, index);   
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(res, exp));
    return true;
}

bool assert_get_u32_error(ll_LinkedList *ll, int index, ll_Error exp) {
    u32 res;
    ll_Error status = ll_get(ll, &res, index);
    EXPECT_TRUE(CUU_ASSERT(status == exp));
    return true;
}

bool assert_remove_u32(ll_LinkedList *ll, int index, u32 exp) {
    u32 res;
    ll_Error status = ll_remove(ll, &res, index);
    EXPECT_TRUE(CUU_ASSERT(status == LL_OK));
    EXPECT_TRUE(CUU_ASSERT_EQ_U32(res, exp));
    return true;
}

bool assert_remove_u32_error(ll_LinkedList *ll, int index, ll_Error exp) {
    u32 res; 
    ll_Error status = ll_remove(ll, &res, index);
    EXPECT_TRUE(CUU_ASSERT(status == exp));
    return true;
}

/// returns
///     LL_OK
//      || LL_ERROR_NULL_LINKED_LIST_POINTER
//      || LL_ERROR_INSUFFICIENT_SIZE if the string couldn't be written to
//      || LL_ERROR_INTERNAL
ll_Error ll_display_u32(const ll_LinkedList *self, char *res, int res_len) {
    if (self == NULL) return LL_ERROR_NULL_LINKED_LIST_POINTER;
    if (res_len < 1) return LL_ERROR_INSUFFICIENT_SIZE;
    if (!EXPECT_S(has_valid_head_tail_state, self, bool)) ERROR_RETURN(LL_ERROR_INTERNAL);

    char *res_cur = res;
    strncpy(res_cur++, "[", res_len);

    for(struct ll_LinkedListNode *n = self->head; n != NULL; n = n->next) {
        u32 data = *(u32*)n->data;
        int remaining_len = res_len - (res_cur - res);

        int chars_written;
        if (n->next == NULL) {
            // don't include a trailing comma for last element
            chars_written = snprintf(res_cur, remaining_len, "%d", data);
        } else {
            chars_written = snprintf(res_cur, remaining_len, "%d, ", data);
        }
        
        if (chars_written > remaining_len) return LL_ERROR_INSUFFICIENT_SIZE;
        if (chars_written < 0) ERROR_RETURN(LL_ERROR_INTERNAL);
        res_cur += chars_written;
    }
    int remaining_len = res_len - (res_cur - res);

    if (remaining_len < 1) return LL_ERROR_INSUFFICIENT_SIZE;
    strncat(res_cur++, "]", remaining_len);
    return LL_OK;
}

void test_pushpop(void) {
    ll_LinkedList *ll = assert_new(/*data_size*/ 4);
    ll_Error status;

    CUU_ASSERT(assert_push_u32(ll, /*val*/ 6000));
    CUU_ASSERT_PTR_NOT_NULL(ll->head);
    CUU_ASSERT_EQ_U32(*((u32*)ll->head->data), 6000);

    CUU_ASSERT(assert_pop_u32(ll, /*exp*/ 6000));

    // assert stack behavior
    assert_pop_empty_u32(ll);
    assert_push_u32(ll, 0xAAAABBBB);
    assert_push_u32(ll, 0xCCCCDDDD);
    assert_push_u32(ll, 0xEEEEFFFF);
    assert_pop_u32(ll, 0xEEEEFFFF);
    assert_pop_u32(ll, 0xCCCCDDDD);
    assert_pop_u32(ll, 0xAAAABBBB);
    assert_pop_empty_u32(ll);

    // test pushing a specific value but poping it without reading content
    CUU_ASSERT(assert_push_u32(ll, 1));
    int out = 555;
    status = ll_pop(ll, NULL);
    CUU_ASSERT(status == LL_OK);
    CUU_ASSERT_EQ_U32(out, 555);
    assert_pop_empty_u32(ll);
    
    status = ll_push(NULL, NULL);
    CUU_ASSERT(status == LL_ERROR_NULL_LINKED_LIST_POINTER);
    status = ll_push(ll, NULL);
    CUU_ASSERT(status == LL_ERROR_NULL_ELEMENT_POINTER);

    ll_free(&ll);
    CUU_ASSERT_PTR_NULL(ll);
}

void test_pushpop_front(void) {
    ll_LinkedList *ll = assert_new(/*data_size*/ 4);
    ll_Error status;

    CU_ASSERT(assert_push_front_u32(ll, /*val*/ 6000));
    CU_ASSERT_PTR_NOT_NULL(ll->head);
    CUU_ASSERT_EQ_U32(ll->len, 1);
    CUU_ASSERT_EQ_U32(*((u32*)ll->head->data), 6000);

    CU_ASSERT(assert_pop_front_u32(ll, /*exp*/ 6000));

    // assert queue behavior
    assert_pop_empty_u32(ll);
    assert_push_front_u32(ll, 1111);
    assert_push_front_u32(ll, 2222);
    assert_push_front_u32(ll, 3333);
    assert_pop_u32(ll, 1111);
    assert_pop_u32(ll, 2222);
    assert_pop_u32(ll, 3333);
    assert_pop_empty_u32(ll);

    // assert stack behavior
    assert_pop_empty_u32(ll);
    assert_push_front_u32(ll, 1111);
    assert_push_front_u32(ll, 2222);
    assert_push_front_u32(ll, 3333);
    assert_pop_front_u32(ll, 3333);
    assert_pop_front_u32(ll, 2222);
    assert_pop_front_u32(ll, 1111);
    assert_pop_front_empty_u32(ll);

    // test pushing a specific value but poping it without reading content
    CUU_ASSERT(assert_push_u32(ll, 1));
    int out = 555;
    status = ll_pop_front(ll, NULL);
    CUU_ASSERT(status == LL_OK);
    CUU_ASSERT_EQ_U32(out, 555);
    assert_pop_empty_u32(ll);
    
    status = ll_push_front(NULL, NULL);
    CUU_ASSERT(status == LL_ERROR_NULL_LINKED_LIST_POINTER);
    status = ll_push_front(ll, NULL);
    CUU_ASSERT(status == LL_ERROR_NULL_ELEMENT_POINTER);
    
    ll_free(&ll);
    CUU_ASSERT_PTR_NULL(ll);
}

void test_iterate_to(void) {
    ll_LinkedList *ll = assert_new(/*data_size*/ 4);

    assert_iterate_to_u32_error(ll, /*index*/ 0, /*exp*/ LL_ERROR_EMPTY_LINKED_LIST);
    assert_push_u32(ll, /*exp*/ 11);
    assert_iterate_to_u32_error(ll, /*index*/ -1, /*exp*/ LL_ERROR_INDEX_OUT_OF_BOUNDS);
    assert_iterate_to_u32_error(ll, /*index*/ 1, /*exp*/ LL_ERROR_INDEX_OUT_OF_BOUNDS);
    assert_iterate_to_u32_error(NULL, /*index*/ 1, /*exp*/ LL_ERROR_NULL_LINKED_LIST_POINTER);
    assert_pop_u32(ll, /*exp*/ 11);
    assert_pop_empty_u32(ll);

    assert_push_u32(ll, /*exp*/ 5);
    assert_iterate_to_u32(ll, /*index*/ 0, /*exp*/ 5);
    assert_push_u32(ll, /*exp*/ 6);
    assert_push_u32(ll, /*exp*/ 7);
    assert_push_u32(ll, /*exp*/ 8);
    assert_iterate_to_u32(ll, /*index*/ 2, /*exp*/ 7);
    assert_pop_front_u32(ll, /*exp*/ 5);
    assert_iterate_to_u32(ll, /*index*/ 0, /*exp*/ 6);
    
    ll_free(&ll);
    CUU_ASSERT_PTR_NULL(ll);
}

void test_insert_get_set_remove(void) {
    ll_LinkedList *ll = assert_new(/*data_size*/ 4);

    // initial insert get remove test
    CUU_ASSERT(assert_pop_empty_u32(ll));
    CUU_ASSERT(assert_insert_u32(ll, 0, 0x100));
    CUU_ASSERT(assert_get_u32(ll, 0, 0x100));
    CUU_ASSERT(assert_remove_u32(ll, 0, 0x100));
    CUU_ASSERT(assert_pop_empty_u32(ll));

    // using insert and remove like push_front and pop_front
    CUU_ASSERT(assert_insert_u32(ll, 0, 0x111));
    CUU_ASSERT(assert_insert_u32(ll, 0, 0x222));
    CUU_ASSERT(assert_insert_u32(ll, 0, 0x333));
    CUU_ASSERT(assert_remove_u32(ll, 0, 0x333));
    CUU_ASSERT(assert_remove_u32(ll, 0, 0x222));
    CUU_ASSERT(assert_remove_u32(ll, 0, 0x111));
    CUU_ASSERT(assert_pop_empty_u32(ll));

    // using insert like a push to tail
    CUU_ASSERT(assert_insert_u32(ll, 0, 0));
    CUU_ASSERT(assert_insert_u32(ll, 1, 1));
    CUU_ASSERT(assert_insert_u32(ll, 2, 2));
    CUU_ASSERT(assert_set_u32(ll, 0, 0x111));
    CUU_ASSERT(assert_set_u32(ll, 1, 0x222));
    CUU_ASSERT(assert_set_u32(ll, 2, 0x333));
    CUU_ASSERT(assert_set_u32_error(ll, 3, 0x404, LL_ERROR_INDEX_OUT_OF_BOUNDS));
    CUU_ASSERT(assert_insert_u32_error(ll, 4, 0x404, LL_ERROR_INDEX_OUT_OF_BOUNDS)); // beyond tail->next!
    CUU_ASSERT(assert_get_u32(ll, 0, 0x111));
    CUU_ASSERT(assert_get_u32(ll, 2, 0x333));
    CUU_ASSERT(assert_get_u32_error(ll, 3, LL_ERROR_INDEX_OUT_OF_BOUNDS)); // can't get tail->next!
    CUU_ASSERT(assert_remove_u32_error(ll, 3, LL_ERROR_INDEX_OUT_OF_BOUNDS)); // can't remove tail->next!
    CUU_ASSERT(assert_remove_u32(ll, 1, 0x222));
    CUU_ASSERT(assert_remove_u32(ll, 1, 0x333));
    CUU_ASSERT(assert_remove_u32(ll, 0, 0x111));
    CUU_ASSERT(assert_pop_empty_u32(ll));

    ll_free(&ll);
    CUU_ASSERT_PTR_NULL(ll);
}

void test_display(void) {
    // initialize seed
    srand(time(NULL));

    ll_LinkedList *ll = assert_new(4);

    for (int i=0; i<10; i++) {
        u32 rand_num = rand() % 50;
        ll_push(ll, &rand_num);
    }
    char s[256];
    if (ll_display_u32(ll, s, 255) != LL_OK) return;
    printf("%s", s);

    ll_free(&ll);
}

int init_suite(void) {
    return 0;
}

int clean_suite(void) {
    return 0;
}

int main (void) {

    CU_pSuite suites[1] = {NULL};
    CU_ErrorCode status = CUE_SUCCESS;

    // Initialize CUnit test registry
    status = CU_initialize_registry();
    if (status != CUE_SUCCESS) return CU_get_error();

    // Add suites to registry
    status = CUU_utils_try_add_suite(&suites[0], "LinkedList Tests");
    if (status != CUE_SUCCESS) return status;

    // add tests to suites
    status = CUU_utils_try_add_test(suites[0], test_pushpop, "\n\nTesting " STR(test_pushpop) "()\n\n");
    status = CUU_utils_try_add_test(suites[0], test_pushpop_front, "\n\nTesting " STR(test_pushpop_front) "()\n\n");
    status = CUU_utils_try_add_test(suites[0], test_iterate_to, "\n\nTesting " STR(test_iterate_to) "()\n\n");
    status = CUU_utils_try_add_test(suites[0], test_insert_get_set_remove, "\n\nTesting " STR(test_insert_get_set_remove) "()\n\n");
    status = CUU_utils_try_add_test(suites[0], test_display, "\n\nTesting " STR(test_display) "()\n\n");
    if (status != CUE_SUCCESS) return status;

    CU_basic_run_tests(); // OUTPUT to the screen
    CU_cleanup_registry(); //Cleaning the Registry

    return CU_get_error();
}

#endif

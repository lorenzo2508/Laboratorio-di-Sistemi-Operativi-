
//
// Created by lorenzo on 15/9/22.
//

#ifndef linkedList_h
#define linkedList_h

#include "node.h"



// DATA TYPES

typedef struct linkedList{
    // Head points to the first node in the list.
    node_t *head;
    node_t *tail; 
    // Length refers to the number of nodes in the list.
    int length;
   
} linkedList_t;



linkedList_t *linked_list_create();

void linked_list_destroy(linkedList_t *linked_list);

//  use by order_insertion, for insert the entry 
//  in case of failure it will return null
void *linkedList_insert_order(linkedList_t *linked_list, int index, void *data, long value, unsigned long size);

//  insert the data of size "size" in the given linked_list at the specified index
//  in case of failure it will return null
void *linkedList_insert(linkedList_t *linked_list, int index, void *data, unsigned long size);

// Remove an item from the list and handles the deallocation of memory.
//  in case of failure it will return null
void *linkedList_remove_node(linkedList_t *linked_list, int index);

// linkedList_access_data allows data in the list to be accessed
// in case of failure it will return null
void *linkedList_access_data (linkedList_t *linked_list, int index);

/* Insert entry in a linked list by ascending order
    in case of failure it will return null*/
void *order_insertion (linkedList_t *linked_list, long value, char *path); 

#endif /* linkedList_h */
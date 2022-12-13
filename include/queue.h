
#ifndef queue_h
#define queue_h

//
// Created by lorenzo on 16/9/22.
//


#include "linkedList.h"
#include "task.h"
#include <pthread.h>

// DATA TYPES

// Queues are used to access a linked list in a controlled manner.
typedef struct queue{
    // A reference to the embedded LinkedList.
    linkedList_t *list;
    int queue_max_bound; 
    pthread_mutex_t queue_lock; 
    pthread_cond_t queue_cond; 

   
} queue_t;



// The queue_create should be used to create new Queue instances.
queue_t *queue_create(int queue_len);
// The destructor should be used to delete a Queue instance.
void queue_destroy(queue_t *queue);
// The push function adds a node to the end of the chain.
void *push(queue_t *queue, void *data, unsigned long size);
// The take function return a pointer to the data from the front of the list.
void *take(queue_t *queue);
// The pop function removes the first item in the list.
void pop(queue_t *queue);

#endif /* queue_h */
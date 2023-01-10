
#include "../include/queue.h"
#include <stdio.h>
#include <stdlib.h> 
#include <errno.h>

// FUNCTION PROTOTYPES

queue_t *queue_create(int queue_len);
void *push(queue_t *queue, void *data, unsigned long size);
void * take(queue_t *queue);
void pop (queue_t *queue);




queue_t *queue_create(int queue_len){

    if(queue_len == 0){
        errno = EINVAL; 
        perror("queue; queue_create"); 
        fprintf(stderr, "Queue's length can't be 0. Err code: %d\n", errno); 
        return NULL; 
    }

    // Create a Queue instance.
    queue_t *queue  = (queue_t*)malloc((queue_len + 1)*sizeof(queue_t)); 
    if(queue == NULL){
        perror ("malloc fail queue_create"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        return NULL; 
    }
    queue->queue_max_bound = queue_len; 

    // Instantiate the queue's LinkedList via the list_create.
    queue->list = linked_list_create();
    queue->queue_lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    if((pthread_cond_init(&queue->queue_cond, NULL)) != 0){
        errno = EACCES; 
        perror("pthread_cond_init"); 
        fprintf(stderr, "Err code: %d\n", errno); 
        exit(EXIT_FAILURE); 
    }
    
    return queue;
}

void queue_destroy(queue_t *queue){
    linked_list_destroy(queue->list);
    free(queue);
}

// The push method adds an item to the end of the list.
void *push(queue_t *queue, void *data, unsigned long size){

    if(queue == NULL){
        errno = EINVAL;
        perror("queue; push. Need to pass a pointer to a queue type object"); 
        fprintf(stderr, "Err code: %d \n", errno); 
        return NULL;
    }

    if(data == NULL){
        errno = EINVAL;
        perror("queue; push. Data to insert can't be NULL"); 
        fprintf(stderr, "Err code: %d \n", errno); 
        return NULL;
    }

    if(size == 0){
        errno = EINVAL;
        perror("queue; push. size of data can't be 0");  
        fprintf(stderr, "Err code: %d \n", errno); 
        return NULL;
    }

    // Utilize insert from LinkedList with enforced parameters.
    
    linkedList_insert(queue->list, queue->list->length, data, size);
    return 0; // Use to remove the Warning: control reaches end of non-void function
   
    
}

// The take function return a pointer to the data from the front of the list.
void *take(queue_t *queue){
    // Utilize the access_data function from linkedList with enforced parameters.
    node_t *access_node = linkedList_access_data(queue->list, 0);
    
    return access_node->data;
}

// The pop function removes the first item in the list.
void pop(queue_t *queue){
    // Utilize the remove function from linkedList with enforced parameters.
    
    linkedList_remove_node(queue->list, 0);
    
}
//
// Created by lorenzo on 19/9/22.
//

#ifndef PROGETTOFARM_SOL22_23_MASTER_THREAD_H

#define PROGETTOFARM_SOL22_23_MASTER_THREAD_H
#include "queue.h"
#include "task.h"
#include "thread_pool.h"
#include <pthread.h>

// DATA TYPES


typedef struct master_args{
    linkedList_t *file_list; 
    // A queue to store work.
    thread_pool_t *thread_pool; 
    queue_t *queue;
    int delay_request_time;
    int number_of_files;  
} master_args_t; 

// Create the master thread, it will enqueue all the task for the threadpool
pthread_t *master_create (master_args_t *master_args, queue_t *queue);

//Destroy the master thread, a pointer to the master_thread and his args need to be pass
void master_thread_destroy(pthread_t *master); 

#endif //PROGETTOFARM_SOL22_23_MASTER_THREAD_H

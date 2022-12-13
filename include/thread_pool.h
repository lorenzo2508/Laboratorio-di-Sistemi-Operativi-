//
// Created by lorenzo on 26/9/22.
//

#ifndef thread_pool_h
#define thread_pool_h

#include "queue.h"
#include "task.h"
#include <pthread.h>

// DATA TYPES

typedef struct thread_pool{
    // The number of threads in the pool.
    int thread_num;
    // variable that tells if the pool has to be active or not 
    int active;
    // A queue to store work.
    queue_t *queue;
    
    pthread_t *pool; //array of thread 

    // making the pool thread-safe.
    pthread_mutex_t lock;
    pthread_cond_t signal;

}thread_pool_t;


// Create the pool, args: number of thread in the pool
thread_pool_t *thread_pool_create(int num_threads, queue_t *queue);
// First arg is the func, the second one is the argument for the func
task_t task_create(void * (*task_function)(char *arg), char *arg);

// Destroy the pool, a pointer to the pool need to be pass
void thread_pool_destroy(thread_pool_t *thread_pool);

#endif // thread_pool_h 
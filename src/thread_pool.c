#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "../include/thread_pool.h"


//  FUNCTION PROTOTYPES 

thread_pool_t *thread_pool_create(int thread_num, queue_t *queue); // Create the pool
void *thread_task(void *arg); // Exec the tasks store inside the queue 
void thread_pool_destroy(thread_pool_t *thread_pool);// Invoke for destroy the pool 

extern volatile sig_atomic_t sig_pipe_rise; 
  

thread_pool_t *thread_pool_create(int thread_num, queue_t *queue){

    thread_pool_t *thread_pool = (thread_pool_t *)malloc (sizeof(thread_pool_t)); 
    if(thread_pool == NULL){
        perror ("malloc fail thread_pool_create; thread_pool"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }

    thread_pool->pool = (pthread_t *) malloc (sizeof(pthread_t)*thread_num);
    if(thread_pool->pool == NULL){
        perror ("malloc fail thread_pool_create; thread_pool->pool"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d", errno);  
        exit(EXIT_FAILURE); 
    }
    thread_pool->thread_num = thread_num; 
    thread_pool->queue = queue; 
    thread_pool->lock = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; 
    thread_pool->signal = (pthread_cond_t) PTHREAD_COND_INITIALIZER; 
    pthread_mutex_lock(&thread_pool->lock);
    for (int i = 0; i < thread_num; i++){
        //fprintf(stderr, "flag activ in pool create %d\n", thread_pool->active);
        if((pthread_create(&(thread_pool->pool[i]), NULL, &thread_task, (void*) thread_pool)) != 0){
            perror("Fail during threadPool creation"); 
            exit(EXIT_FAILURE); 
        }
        
    }
    pthread_mutex_unlock(&thread_pool->lock);



    return thread_pool; 
}


void *thread_task(void *arg){
    thread_pool_t *thread_pool = (thread_pool_t*) arg;
    thread_pool->active = 1; 
   

    while(1){
       
        if(thread_pool->active == 0){
            break;
        }

        if(sig_pipe_rise == 1){
            pthread_kill(pthread_self(), SIGPIPE); 
            continue;
        }

       
        pthread_mutex_lock(&thread_pool->queue->queue_lock); 
     
        while(thread_pool->queue->list->length == 0){
            pthread_cond_wait(&thread_pool->queue->queue_cond, &thread_pool->queue->queue_lock);
        }
      
        // Check if there is something in the queue to be taken
        
        task_t *thread_task = (task_t*) take(thread_pool->queue); 
       
       if((strcmp(thread_task->arg, "stop")) == 0){
            thread_pool->active = 0;
            push(thread_pool->queue, (void*)thread_task, sizeof(task_t));
            pthread_cond_broadcast(&thread_pool->queue->queue_cond);
            pthread_mutex_unlock(&thread_pool->queue->queue_lock);
            continue;
        }
       
        
        //exec task 
        thread_task->taskfunc(thread_task->arg);
        // Pull out the task from the queue 
     

        pop(thread_pool->queue);  

        pthread_cond_broadcast(&thread_pool->queue->queue_cond);
        pthread_mutex_unlock(&thread_pool->queue->queue_lock);
       
       

    }

    return NULL;

}

void thread_pool_destroy(thread_pool_t *thread_pool){
    
    for (int i = 0; i < thread_pool->thread_num; i++)
    {
        pthread_join(thread_pool->pool[i], NULL);
    }
    free(thread_pool->pool);
    free(thread_pool);
}



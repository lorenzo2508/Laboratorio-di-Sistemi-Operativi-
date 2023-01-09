#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <time.h>
#include "../include/master_thread.h"
#include "../include/util.h"


#define BUFSZ 8
#define MAXFILENAME 2048
#define MAXPATH 4096
#define BUFSIZE 256


extern volatile sig_atomic_t partial_result; 
extern volatile sig_atomic_t no_more_task;
extern volatile sig_atomic_t sig_pipe_rise; 


// FUNCTION PROTOTYPES

void *enqueue_task(void *arg);
void *workerTask (char *filepath);
void master_thread_destroy(pthread_t *master_thread);
pthread_t *master_create (master_args_t *master_args, queue_t *queue);

// Create the master thread, it will enqueue all the task for the threadpool
pthread_t *master_create (master_args_t *master_args, queue_t *queue){
    
    pthread_t *master_thread = (pthread_t *)malloc (sizeof(pthread_t)); 
    if(master_thread == NULL){
        perror ("malloc fail master_create"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    master_args_t *args = master_args; 
    args->queue = queue; 
    if((pthread_create(master_thread, NULL, &enqueue_task, args) != 0)){
        errno = EACCES;
        perror("Fail during threadPool creation"); 
        fprintf(stderr, "errno value: %d", errno);   
        return NULL;  
    }


    return master_thread; 
}

// this is the task that a worker of the pool has to execute
void *workerTask (char *filepath){
    
    
    if(filepath == NULL){
        errno = EINVAL; 
        exit(EXIT_FAILURE); 
    }
        
    char sockname[MAXPATH] = "./farm.sck"; 
    // Print task. Notify to collector that he has to print partial results 
    if(strcmp(filepath, "print") == 0){
   
    
    long fd_socket = socket(AF_UNIX, SOCK_STREAM, 0); 
    if(fd_socket == -1){
        perror("Fail during socket creation"); 
        fprintf(stderr, "errno code %d", errno); 
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un saddr;
    memset(&saddr, '0', sizeof(saddr));
    strcpy(saddr.sun_path, sockname);
    saddr.sun_family = AF_UNIX;

    char buf_send_to_collector[MAXFILENAME]; 
    memset(buf_send_to_collector, 0, MAXFILENAME); 

    snprintf(buf_send_to_collector, MAXFILENAME, "%s", filepath);
    //fprintf(stderr, "sopra la connect 1\n");
    while((connect(fd_socket, (struct sockaddr*) &saddr, sizeof(saddr))) == -1){
        if ( errno == ENOENT ) {
            if((usleep(1000000)) == -1){
                perror("usleep");
                fprintf(stderr, "Err code: %d\n", errno); 
                exit(EXIT_FAILURE);
            }
        }
            
        else exit(EXIT_FAILURE);             
    }
    
    //fprintf(stderr, "scrivo exit, buf: %s \n", buf_send_to_collector); 
    if((writen(fd_socket, buf_send_to_collector, MAXFILENAME)) == -1){
        perror("writen"); 
        close(fd_socket); 
        exit(EXIT_FAILURE); 
    }
    close(fd_socket);
    return NULL; 
    
    }


        // Exit task. Notify to collector that he has to end his while loop
    if(strcmp(filepath, "exit") == 0){
        
        
        long fd_socket = socket(AF_UNIX, SOCK_STREAM, 0); 
        if(fd_socket == -1){
            perror("Fail during socket creation"); 
            fprintf(stderr, "errno code %d", errno); 
            exit(EXIT_FAILURE);
        }
        struct sockaddr_un saddr;
        memset(&saddr, '0', sizeof(saddr));
        strcpy(saddr.sun_path, sockname);
        saddr.sun_family = AF_UNIX;

        char buf_send_to_collector[MAXFILENAME]; 
        memset(buf_send_to_collector, 0, MAXFILENAME); 

        snprintf(buf_send_to_collector, MAXFILENAME, "%s", filepath);
        while((connect(fd_socket, (struct sockaddr*) &saddr, sizeof(saddr))) == -1){
            if ( errno == ENOENT ){
                if((usleep(1000000)) == -1){
                    perror("usleep");
                    fprintf(stderr, "Err code: %d\n", errno); 
                    exit(EXIT_FAILURE);
                }
            } 
    
            else exit(EXIT_FAILURE); 
        }
        //fprintf(stderr, "scrivo exit, buf: %s \n", buf_send_to_collector); 
        if((writen(fd_socket, buf_send_to_collector, MAXFILENAME)) == -1){
            perror("writen"); 
            close(fd_socket); 
            exit(EXIT_FAILURE); 
        }
        close(fd_socket);
        return NULL; 
    
    }
    
    //fprintf(stderr, "sto per aprire il file %s\n", filepath);
    FILE *fp = fopen(filepath, "rb"); 
    if (fp == NULL) {
        perror("fopen");
        fprintf(stderr, "Err code: %d \n", errno);
        exit(EXIT_FAILURE);
    }
    unsigned char buf[BUFSZ] = {0};
    size_t bytes = 0, 
    readsz = sizeof buf;
    

        

    //===================================================================================
    // Acquire the file size

    if((fseek(fp, 0L, SEEK_END)) == -1){
        perror("fseek"); 
        fprintf(stderr, "Err code: %d \n", errno);
        exit(EXIT_FAILURE);

    } // going to the end of the file for calculating the dimension 

    // ftell gives me the size of the file in byte 
    long int file_size = ftell(fp); 
    if(file_size == -1){
        perror("ftell"); 
        fprintf(stderr, "Err code: %d \n", errno);
        exit(EXIT_FAILURE);
    }
    rewind(fp); // Go back to the beginning of the file, it returns no value so doesn't need error handling
    

    //===================================================================================
    // Execute the requested task, the sum of the long contained in the file

    int cont = 0; 
    long ris = 0; 
    while ((bytes = fread (buf, sizeof *buf, readsz, fp)) == readsz) {
        // here the possible error derived from "fread" is handled as shown in the linux man page for fread
        if (bytes != readsz) {
            fprintf(stderr, "fread() failed: %zu\n", bytes);
            exit(EXIT_FAILURE);
        }
        ris = ris + ((long)buf[0] * cont); 
        cont ++;
        
    }

    fclose(fp);

    //===================================================================================
    // Connection to the socket 
 
    
    long fd_socket = socket(AF_UNIX, SOCK_STREAM, 0); 
    if(fd_socket == -1){
        perror("Fail during socket creation"); 
        fprintf(stderr, "errno code %d", errno); 
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un saddr;
    memset(&saddr, '0', sizeof(saddr));
    strcpy(saddr.sun_path, sockname);
    saddr.sun_family = AF_UNIX;

    char buf_send_to_collector[MAXFILENAME]; 
    memset(buf_send_to_collector, 0, MAXFILENAME); 

    snprintf(buf_send_to_collector, MAXFILENAME, "%ld %s", ris, filepath);
   
    while((connect(fd_socket, (struct sockaddr*) &saddr, sizeof(saddr))) == -1){
        if ( errno == ENOENT ){
                if((usleep(1000000)) == -1){
                perror("usleep");
                fprintf(stderr, "Err code: %d\n", errno); 
                exit(EXIT_FAILURE);
            }
        }
        else exit(EXIT_FAILURE); 
    }
    //fprintf(stderr, "sopra la write 3\n");
    if( (writen(fd_socket, buf_send_to_collector, MAXFILENAME)) == -1){
        perror("writen"); 
        close(fd_socket); 
        exit(EXIT_FAILURE); 
    }
    //fprintf(stderr, "sopra la close 3, buff: %s\n", buf_send_to_collector);
    close(fd_socket); 
    return NULL; 

}


void *enqueue_task(void *arg){
    master_args_t *args = (master_args_t*) arg; 
    task_t *task = (task_t *)malloc(sizeof (task_t));
    if(task == NULL){
        perror ("malloc fail enqueue task; task"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    int i = 0;  
    int delay = args->delay_request_time * 1000; 
    while(1){
        //  Delay the request
        if((usleep(delay)) == -1){
            perror ("usleep"); 
            fprintf(stderr, "code: %d\n", errno); 
            exit(EXIT_FAILURE);
        }

        if(i < args->number_of_files){
            node_t *entry = linkedList_access_data(args->file_list, i); // Use to take the entry of master_args list that contain filename
            if(entry == NULL){
                errno = EINVAL; 
                perror("Master thread linked_list_access_data"); 
                fprintf(stderr, "Err code: %d\n", errno); 
                exit(EXIT_FAILURE);
            }
            task->arg = (char*) entry->data; 
            task->taskfunc = workerTask; 
        }
        pthread_mutex_lock(&args->queue->queue_lock); 
        //fprintf(stderr, "queue len: %d\n", args->queue->list->length);
        // if SIGPIPE rise then master enqueue stop task to stop the pool    
        //signal handling
         if(sig_pipe_rise == 1){
            
            task_t *end_taks = (task_t*)malloc(sizeof(task_t)); // Use by threadpool to stop all threads
            if(end_taks ==NULL){
                perror ("malloc fail enqueue task; end_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }
            end_taks->arg = "stop";
            end_taks->taskfunc = workerTask;
            push(args->queue, (void*)end_taks, sizeof(task_t));
            free(end_taks);
            pthread_cond_broadcast(&args->queue->queue_cond);
            pthread_mutex_unlock(&args->queue->queue_lock); 
            break;  
        }
        // When master_thread has enqueue all the task, enqueu those two special tasks
        // end_task and exit_task
        if(i == args->number_of_files){
            task_t *end_taks = (task_t*)malloc(sizeof(task_t)); // Use by threadpool to stop all threads
            if(end_taks ==NULL){
                perror ("malloc fail enqueue task; end_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }
            task_t *exit_taks = (task_t*)malloc(sizeof(task_t)); // Use by collector process to exit his while loop
            if(exit_taks ==NULL){
                perror ("malloc fail enqueue task; exit_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }
            //fprintf(stderr, "creo exit \n"); 
            exit_taks->arg = "exit";
            exit_taks->taskfunc = workerTask;  
            end_taks->arg = "stop";
            end_taks->taskfunc = workerTask;
            //fprintf(stderr, "accodo exit \n"); 
            
            // Put the special tasks in the queue 
            push(args->queue, (void*)exit_taks, sizeof(task_t)); 
            push(args->queue, (void*)end_taks, sizeof(task_t));
            free(exit_taks);
            free(end_taks);
            pthread_mutex_unlock(&args->queue->queue_lock); 
            pthread_cond_broadcast(&args->queue->queue_cond);
            break; 
        }
        
        // If some signals (SIGINT, SIGQUIT, SIGHUP, SIGTERM) set "no_more_task"
        // the master_thread enqueue the special tasks to stop the pool and notify that collector has to exit his while loop
        // then master_thread stops. 
        //signal handling
        if(no_more_task == 1){
            task_t *end_taks = (task_t*)malloc(sizeof(task_t)); // Use by threadpool to stop all threads
             if(end_taks ==NULL){
                perror ("malloc fail enqueue task; end_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }

            task_t *exit_taks = (task_t*)malloc(sizeof(task_t)); // Use to notify collector process that has to exit his while loop
            if(exit_taks ==NULL){
                perror ("malloc fail enqueue task; exit_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }
            //fprintf(stderr, "creo exit \n"); 
            exit_taks->arg = "exit";
            exit_taks->taskfunc = workerTask;  
            end_taks->arg = "stop";
            end_taks->taskfunc = workerTask;
            //fprintf(stderr, "accodo exit \n"); 
            
            // Put the special tasks in the queue 
            push(args->queue, (void*)exit_taks, sizeof(task_t));
            push(args->queue, (void*)end_taks, sizeof(task_t));
            free(exit_taks);
            free(end_taks);
            pthread_mutex_unlock(&args->queue->queue_lock); 
            pthread_cond_broadcast(&args->queue->queue_cond);
            break; 

        }
        // partial_result (SIGUSR1 arrive)
        if(partial_result == 1){
            task_t *print_taks = (task_t*)malloc(sizeof(task_t)); // Use to notify collector process to print partial results 

            if(print_taks ==NULL){
                perror ("malloc fail enqueue task; print_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }
            //fprintf(stderr, "creo exit \n"); 
            print_taks->arg = "print";
            print_taks->taskfunc = workerTask;  
            push(args->queue, (void*)print_taks, sizeof(task_t));
            free(print_taks);
            pthread_mutex_unlock(&args->queue->queue_lock); 
            pthread_cond_broadcast(&args->queue->queue_cond);
        }

        // Wait on this condition: if the queue's length is equal to the bound 
        // we have to wait for the pool to consume at least one task  
        while(args->queue->list->length == args->queue->queue_max_bound){
            pthread_cond_wait(&args->queue->queue_cond, &args->queue->queue_lock); 
        }
      

        // Put the task in the queue 
        push(args->queue, (void*)task, sizeof(task_t));
        i++;
      
     

        pthread_mutex_unlock(&args->queue->queue_lock); 
        pthread_cond_broadcast(&args->queue->queue_cond);

    }

    free(task); 
    return NULL; 

}

//Destroy the master thread, a pointer to the master need to be pass
void master_thread_destroy(pthread_t *master_thread){
    pthread_join(*master_thread, NULL);
    
    free(master_thread);
}
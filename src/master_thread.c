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
#include "../include/utilities.h"

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
        perror("Fail during threadPool creation"); 
        errno = EACCES;
        fprintf(stderr, "errno value: %d", errno);   
        exit(EXIT_FAILURE); 
    }


    return master_thread; 
}

void *workerTask (char *filepath){
    int err = 0; 
    
    if(filepath == NULL){
        errno = EINVAL; 
        exit(EXIT_FAILURE); 
    }
        
    char sockname[MAXPATH] = "./farm.sck"; 
    // Print task. Notify to collector that he has to print partial results 
    if(strcmp(filepath, "print") == 0){
    int err; 
    
    long fd_socket = socket(AF_UNIX, SOCK_STREAM, 0); 
    if(fd_socket == -1){
        err = errno;
        perror("Fail during socket creation"); 
        fprintf(stderr, "errno code %d", err); 
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
        
        if ( errno == ENOENT ) 
            sleep(1);
        else exit(EXIT_FAILURE);             
    }
    
    //fprintf(stderr, "scrivo exit, buf: %s \n", buf_send_to_collector); 
    write(fd_socket, buf_send_to_collector, MAXFILENAME);
    close(fd_socket);
    return NULL; 
    
    }


        // Exit task. Notify to collector that he has to end his while loop
    if(strcmp(filepath, "exit") == 0){
        int err; 
        
        long fd_socket = socket(AF_UNIX, SOCK_STREAM, 0); 
        if(fd_socket == -1){
            err = errno;
            perror("Fail during socket creation"); 
            fprintf(stderr, "errno code %d", err); 
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
            if ( errno == ENOENT ) 
                sleep(1);
            else exit(EXIT_FAILURE); 
        }
        //fprintf(stderr, "scrivo exit, buf: %s \n", buf_send_to_collector); 
        write(fd_socket, buf_send_to_collector, MAXFILENAME);
        close(fd_socket);
        return NULL; 
    
    }
    
    //fprintf(stderr, "sto per aprire il file %s\n", filepath);
    FILE *fp = fopen(filepath, "rb"); 
    unsigned char buf[BUFSZ] = {0};
    size_t bytes = 0, readsz = sizeof buf;
    if (fp == NULL) {
        perror("File Not Found!\n");
        fprintf(stderr, "here \n");
        exit(EXIT_FAILURE);
    }

        

    //===================================================================================
    // Acquire the file size

    fseek(fp, 0L, SEEK_END); // going to the end of the file for calculating the dimension 

    // ftell gives me the size of the file in byte 
    long int file_size = ftell(fp); 
    rewind(fp); // Go back to the beginning of the file 
    if(file_size == -1){
        perror("Dimension not avaible"); 
        exit(EXIT_FAILURE);
    }

    //===================================================================================
    // Execute the requested task, the sum of the long contained in the file

    int cont = 0; 
    long ris = 0; 
    while ((bytes = fread (buf, sizeof *buf, readsz, fp)) == readsz) {
        ris = ris + ((long)buf[0] * cont); 
        cont ++;
        
    }



    //===================================================================================
    // Now I close the file

    fclose(fp);

    //===================================================================================
    // Connection to the socket 
 
    
    long fd_socket = socket(AF_UNIX, SOCK_STREAM, 0); 
    if(fd_socket == -1){
        err = errno;
        perror("Fail during socket creation"); 
        fprintf(stderr, "errno code %d", err); 
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
        if ( errno == ENOENT ) 
                sleep(1);
        else exit(EXIT_FAILURE); 
    }
    //fprintf(stderr, "sopra la write 3\n");
    if( (write(fd_socket, buf_send_to_collector, MAXFILENAME)) == -1){
        perror("write"); 
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
            task_t *exit_taks = (task_t*)malloc(sizeof(task_t)); // Use by collector process to exit his while loop
            if(end_taks ==NULL){
                perror ("malloc fail enqueue task; end_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }
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
            task_t *exit_taks = (task_t*)malloc(sizeof(task_t)); // Use to notify collector process that has to exit his while loop
            if(end_taks ==NULL){
                perror ("malloc fail enqueue task; end_task"); 
                errno = ENOMEM;
                fprintf(stderr, "errno value: %d\n", errno);  
                exit(EXIT_FAILURE); 
            }
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
        //signal handling
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
      //fprintf(stderr, "pusho il task \n");

        // Put the task in the queue 
        push(args->queue, (void*)task, sizeof(task_t));
        i++;
      
      //fprintf(stderr, "rilascio le lock e chiamo la broadcast\n");

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
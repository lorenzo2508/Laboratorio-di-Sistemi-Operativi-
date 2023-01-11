#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include "../include/linkedList.h"
#include "../include/queue.h"
#include "../include/thread_pool.h"
#include "../include/master_thread.h"
#include "../include/util.h"

#if !defined(MAXFILENAME)
#define MAXFILENAME 2048
#endif
#define SOCKMAXPATH 108
#define BUFSIZE 256


volatile sig_atomic_t partial_result; 
volatile sig_atomic_t no_more_task; 
volatile sig_atomic_t sig_pipe_rise; 

pthread_t signal_thread_id = -1; 

//  FUNCTION PROTOTYPES

int isdot(const char dir[]);
void lsR(const char nomedir[], linkedList_t *dir_file_list) ;
static void *signal_handler_func (void *args);


//  Signal-handler function, it will be use by a thread to handle the signal 
//signal handling
static void *signal_handler_func (void *args){
    sigset_t* set = (sigset_t*) args; //    use to identify the signal set
    int signal; //  this is the signal recive

    signal_thread_id = pthread_self(); 
    //  infinite loop to handle the signal  
    while(1){
        if((sigwait(set, &signal)) != 0){
            perror("sigwait");
            fprintf(stderr, "Fatal Error on sigwait");  
            abort();
        } 
        switch (signal)
        {
        case SIGINT:
            no_more_task = 1; 
            return NULL;
            

        case SIGQUIT:
            no_more_task = 1; 
            return NULL;
           

        case SIGHUP:
            no_more_task = 1; 
            return NULL;
            
        case SIGTERM:
            no_more_task = 1; 
            return NULL;
            
        
        case SIGPIPE:
            sig_pipe_rise = 1; 
            return NULL;
           
        
        case SIGUSR1:
            partial_result = 1;
            //return NULL;
            break; 

        case SIGUSR2: 
            return NULL; 
        
        default:
            break;
        }

        

        
        

    }
}

//  check if the character pass is a dot or not 
//  return 0 if it is a dot else -1
int isdot(const char dir[]) {
  int l = strlen(dir);
  
  if ( (l>0 && dir[l-1] == '.') ) return 1;
  return 0;
}


//  browse folders and subfolders recursively. Return filenames memorised inside a linkedList. 
void lsR(const char nomedir[], linkedList_t *dir_file_list) {
    // controllo che il parametro sia una directory
    struct stat statbuf;
    int r;
    SYSCALL_EXIT(stat,r,stat(nomedir,&statbuf),"Doing stat of the name %s: errno=%d\n",
	    nomedir, errno);

    DIR * dir;
    
    if ((dir=opendir(nomedir)) == NULL) {
	    perror("opendir");
	    fprintf(stderr, "Err doing opendir %s\n", nomedir);
	    return;
    } else {
	struct dirent *file;
    
	while((errno=0, file =readdir(dir)) != NULL) {
	    struct stat statbuf;
	    char filename[MAXFILENAME]; 
	    int len1 = strlen(nomedir);
	    int len2 = strlen(file->d_name);
	    if ((len1+len2+2)>MAXFILENAME) {
		    fprintf(stderr, "ERR: MAXFILENAME too short\n");
		    exit(EXIT_FAILURE);
	    }	    
	    strncpy(filename,nomedir,      MAXFILENAME-1);
	    strncat(filename,"/",          MAXFILENAME-1);
	    strncat(filename,file->d_name, MAXFILENAME-1);
	    
	    if (stat(filename, &statbuf)==-1) {
		perror("Stat");
		fprintf(stderr, "Err in filename %s\n", filename);
		return;
	    }
	    if(S_ISDIR(statbuf.st_mode)) {
		if ( !isdot(filename) ) 
			lsR(filename, dir_file_list);
	    } 
		else {
            // check if the file is regular 
            if(((isRegular (filename, NULL)) == 1))
                linkedList_insert(dir_file_list, dir_file_list->length, (void*)filename, (strlen(filename)*sizeof(char)+5)); 		
                
            
	    }
	}
	if (errno != 0) 
        perror("readdir");
	closedir(dir);
    }
}




int main (int argc, char *argv[]){

    //  For signals handling, is used a dedicate thread, like the example at the man page for "pthread_sigmask(3)"
    //  (see: man pthread_sigmask) 
    //  Declaretion for handling the signals

    pthread_t signal_handler; // thread use to handle signals

   
    sigset_t set; 
    partial_result = 0; 
    no_more_task = 0; 

    EXIT_ON_SIGNAL_HANDLING(sigemptyset(&set), "sigemptyset\n");
    EXIT_ON_SIGNAL_HANDLING(sigaddset(&set, SIGINT), "sigaddset SIGINT");
    EXIT_ON_SIGNAL_HANDLING(sigaddset(&set, SIGHUP), "sigaddset SIGHUP");
    EXIT_ON_SIGNAL_HANDLING(sigaddset(&set, SIGQUIT), "sigaddset SIGQUIT");
    EXIT_ON_SIGNAL_HANDLING(sigaddset(&set, SIGTERM), "sigaddset SIGTERM");
    EXIT_ON_SIGNAL_HANDLING(sigaddset(&set, SIGPIPE), "sigaddset SIGPIPE");
    EXIT_ON_SIGNAL_HANDLING(sigaddset(&set, SIGUSR2), "sigaddset SIGUSR2");
    EXIT_ON_SIGNAL_HANDLING(sigaddset(&set, SIGUSR1), "sigaddset SIGUSR1");

    

    if( (pthread_sigmask(SIG_BLOCK, &set, NULL)) != 0){
        fprintf(stderr, "FATAL ERROR \n"); 
        perror("pthread_sigmask"); 
        abort(); 
    } 

//  Create signal handler thread

    if( (pthread_create(&signal_handler, NULL, &signal_handler_func, (void*) &set)) != 0){
        perror("Signal handler thread creation"); 
        fprintf(stderr, "Signal handler thread creation fail, abort \n"); 
        abort();  
    } 

//=================================================================================
                                //COLLECTOR - child process 
//=================================================================================
//  Creation of the Collector process using fork() syscall 

    pid_t process_id = fork(); 
    if(process_id == -1){
        perror("fork"); 
        exit(EXIT_FAILURE); 
    }
    
    
    
    // process_id = 0 means that we are in the child process (Collector)
   
    if (process_id == 0){
        //=========================================================================
        //  COLLECTOR PROCESS CODE

        // Signal handling: the collector process has to ignore signals
        struct sigaction sig_act;
        memset(&sig_act, 0, sizeof(sig_act));
        sig_act.sa_handler = SIG_IGN; // Signals are ignored by Collector process
        sig_act.sa_flags = SA_RESTART; // I don't want to interrupt any syscall 
        sigset_t handler_mask;
        EXIT_ON_SIGNAL_HANDLING(sigemptyset(&handler_mask), "sigemptyset\n"); 
        EXIT_ON_SIGNAL_HANDLING(sigaddset(&handler_mask, SIGINT), "sigaddset SIGINT");
        EXIT_ON_SIGNAL_HANDLING(sigaddset(&handler_mask, SIGHUP), "sigaddset SIGHUP");
        EXIT_ON_SIGNAL_HANDLING(sigaddset(&handler_mask, SIGQUIT), "sigaddset SIGQUIT");
        EXIT_ON_SIGNAL_HANDLING(sigaddset(&handler_mask, SIGTERM), "sigaddset SIGTERM");
        EXIT_ON_SIGNAL_HANDLING(sigaddset(&handler_mask, SIGPIPE), "sigaddset SIGPIPE");
        EXIT_ON_SIGNAL_HANDLING(sigaddset(&handler_mask, SIGUSR1), "sigaddset SIGUSR1");
        
        sig_act.sa_mask = handler_mask; 
        
        EXIT_ON_SIGNAL_HANDLING(sigaction(SIGINT, &sig_act, NULL), "sigaction SIGINT \n");
        EXIT_ON_SIGNAL_HANDLING(sigaction(SIGHUP, &sig_act, NULL), "sigaction SIGHUP \n");
        EXIT_ON_SIGNAL_HANDLING(sigaction(SIGQUIT, &sig_act, NULL), "sigaction SIGQUIT \n");
        EXIT_ON_SIGNAL_HANDLING(sigaction(SIGTERM, &sig_act, NULL), "sigaction SIGTERM \n");
        EXIT_ON_SIGNAL_HANDLING(sigaction(SIGUSR1, &sig_act, NULL), "sigaction SIGUSR1 \n");
        
         

        // initialize the buffer
        char buff[MAXFILENAME];
        int n_byte_read; 
        // Initialize the list that stores all the results

        linkedList_t *result_list = linked_list_create(); 
        node_t *entry = NULL; // Use to print the fild of the list 
        
        //=======================================================================
        //  Socket creation 
        int fd_sk; // connection socket
        int fd_client; // I/O socket 
        int fd_num = 0; // max fd active 
        int flag_to_stop = 0; 
        int fd_index; // Index to verify select's results 
        struct sockaddr_un sa;

        strcpy(sa.sun_path, "./farm.sck");
        sa.sun_family = AF_UNIX;
        
        fd_sk = socket(AF_UNIX, SOCK_STREAM, 0); 
        if(fd_sk == -1){
            perror("socket"); 
            fprintf(stderr, "Err code: %d\n", errno);
            exit(EXIT_FAILURE); 
        }
        if((bind(fd_sk, (struct sockaddr *)&sa, sizeof(sa))) == -1){
            perror("bind"); 
            fprintf(stderr, "Err code: %d\n", errno);
            exit(EXIT_FAILURE); 
        } 
        if((listen(fd_sk, SOMAXCONN)) == -1){
            perror("listen"); 
            fprintf(stderr, "Err code: %d\n", errno);
            exit(EXIT_FAILURE); 
        }

       
        //===========================================================================
        //  Sets for select 
        fd_set set, ready_set; 
        
        if(fd_sk > fd_num)
            fd_num = fd_sk; 
        FD_ZERO(&set);
        FD_SET(fd_sk, &set);
        
         

        //===========================================================================
        //  While loop for handle the connection 
        while(1){
           ready_set = set; 
        
        
           if((select(fd_num + 1, &ready_set, NULL, NULL, NULL)) == -1){
                perror("select"); 
                exit(EXIT_FAILURE); 
           }
           else{
             
            // Check fd set
            for(fd_index = 0; fd_index <= fd_num; fd_index ++){
                
                if(FD_ISSET(fd_index, &ready_set)){
                    
                    // If fd ready is the server fd ("fd_sk") then accept a new connection 
                    if(fd_index == fd_sk){
                       
                        fd_client = accept(fd_sk, NULL, 0); 
                        if(fd_client == -1){
                            perror("accept"); 
                            fprintf(stderr, "Err code:%d", errno); 
                            exit(EXIT_FAILURE);
                        }
                       
                        FD_SET(fd_client, &set); 
                        if(fd_client > fd_num)
                            fd_num = fd_client; 
                        
                    }
                    else{
                        
                        n_byte_read = read(fd_index, buff, MAXFILENAME); 
                        if(n_byte_read == -1){
                            close(fd_index); 
                            perror("read"); 
                            exit(EXIT_FAILURE); 
                        }
                       
                        if(n_byte_read == 0){
                            FD_CLR(fd_index, &set);
					        if (fd_index == fd_num) fd_num--;
                            close(fd_index); 
                        }
                        else{
                            
                            //  Split the request string 
                            //  (request string = "<long integer> <filename.dat>")
                            char *token; 
                            token = strtok(buff, " ");  //  It splis on the space
                            

                            //  Convert the first string from char* to long 
                            long binary_result = 0;
                            if(isNumber(token, &binary_result) == 2){
                                perror("overflow/underflow"); 
                                errno = ERANGE; 
                                fprintf(stderr, "Err code: %d", errno); 
                                exit(EXIT_FAILURE);   
                            }
                            
                            //  Printing partial result (if SIGSUR1 arise)
                            if(strcmp(token, "print")== 0){
                                for(int j = 0; j < result_list->length; j++){
                                    entry = (node_t*)linkedList_access_data(result_list, j); 
                                    if(entry == NULL){
                                        errno = EINVAL; 
                                        perror("main [farm] linked_list_access_data"); 
                                        fprintf(stderr, "Err code: %d\n", errno); 
                                        exit(EXIT_FAILURE);
                                    }
                                    printf("%ld %s\n", entry->value, (char*)entry->data); 
                                    
                                }
                                continue;
                            }
                                
                            

                            //  If the request string is "exit" then it means that all requests are done, so collector doesn't need anymore to wait on the socket
                            if(strcmp(token, "exit")== 0){
                                flag_to_stop = 1; 
                                break; 
                            }

                            //  Take the second part of the request string (filename)
                            token = strtok(NULL, " ");
                            order_insertion(result_list, binary_result, token); 
            
                        }

                        
                    } 

                } // End of if to check what fd is set 

            } // End of for loop

         }// End of first else
        if(flag_to_stop == 1)
            break;

        } // End of while loop

       

        // Printing the results on stdout 
        for(int i = 0; i < result_list->length; i++){
            entry = (node_t*)linkedList_access_data(result_list, i); 
            if(entry == NULL){
                errno = EINVAL; 
                perror("main [farm] linked_list_access_data"); 
                fprintf(stderr, "Err code: %d\n", errno); 
                return -1;
            }
            printf("%ld %s\n", entry->value, (char*)entry->data); 
        }

        // Free the linkedList use for store and print the results
        linked_list_destroy(result_list);
        
        
        return 0; 
        
    }
 
    //=================================================================================
else{                           // BACK TO
                                // MASTER PROCESS CODE - parent process    
//=================================================================================
//  Declaration 

    long thread_num; 
    long queue_len; 
    int delay_request_time = 0; 
    char source_dir[FILENAME_MAX]; 
    int d_isSet = 0; 
    int num_of_thread_set = 0; 
    int queue_len_set = 0; 
    int check_if_number = 0; 
    int err; 
    //==============================================================================================


//=================================================================================
    // Parsing argv
   
    
    if(argc < 2) {
        printf("Inserire i parametri da riga di comando\n");
        exit(EXIT_FAILURE) ;
    }

    //parsing with getopt

    int opt; 
    int cont_opt = 0; 
    while ((opt = getopt(argc, argv, "n:q:d:t:")) != -1) {
        switch (opt){
            // This option specifies the number of thread in the pool
            case 'n':
                // I use sscanf to read the number of threads from optarg and write it in the variable thread_num
                check_if_number = isNumber(optarg, &thread_num); 
                if(check_if_number == 2){
                    perror("overflow/underflow"); 
                    errno = ERANGE; 
                    fprintf(stderr, "Err code: %d", errno); 
                    exit(EXIT_FAILURE);   
                }
                if(check_if_number == 1){
                    fprintf(stderr, "Need to pass a number as argument of opt -n\n"); 
                    exit(EXIT_FAILURE);
                }
                cont_opt ++;
                num_of_thread_set = 1; 
                //printf("caso -n %d\n", thread_num); 
                break;
            // This option specifies the upper bound of the queue 
            case 'q':
                // I use sscanf to read from optarg the queue's upper bound and I write it in queue_len 
                check_if_number = isNumber(optarg, &queue_len); 
                if(check_if_number == 2){
                    perror("overflow/underflow"); 
                    errno = ERANGE; 
                    fprintf(stderr, "Err code: %d", errno); 
                    exit(EXIT_FAILURE);   
                }
                if(check_if_number == 1){
                    fprintf(stderr, "Need to pass a number as argument of opt -q\n"); 
                    exit(EXIT_FAILURE);
                }
                queue_len_set = 1; 
                cont_opt ++;
                //printf("caso -q %d\n", queue_len); 
                break;
            // This option specifies the dir that contains targets file 
            case 'd':
                // I use sscanf to read from optarg the pathname of the folder that contains the binary files and I write it in source_dir
                sscanf(optarg, "%s", source_dir);
                cont_opt ++;
                d_isSet = 1; 
                //printf("caso -d %s\n", source_dir); 
                break;
            // This option specifies the delay time between two consecutive request
            case 't':
                // I use sscanf to read from optarg the delay time between two successive requests and I write it in delay_request_time
                sscanf(optarg, "%d", &delay_request_time);
                cont_opt ++; 
                
                //printf("caso -t %d\n", delay_request_time); 
                break;
            default:
                break;
            }

    }

    cont_opt = cont_opt * 2; 
    int number_of_file = argc - (cont_opt + 1); 
    
    //  Use this number_of_file's copy for destroy the filename (char **files) array
    //  variable used only to make the code easier to read  
    int number_of_file_cpy_for_destruction = number_of_file; 
   
    //  Create an argv copy (call master_args->files) that contains only filepath
    //  it will contains the file passed by command line
    //  later the file in -d <dir> will be add to master_args->files
    
    int j = 0; // index use for the array call "files"

    
    char **files = malloc((number_of_file)*sizeof(char*));
    if(files == NULL){
        perror ("malloc fail main [farm]; files"); 
        errno=ENOMEM; 
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    for(int i = cont_opt + 1; i < argc; ++i){
        size_t length = strlen(argv[i])+1;
        files[j] = malloc(length * (sizeof(char*)));
        if(files[j] == NULL){
            perror ("malloc fail main [farm]; files[j]"); 
            errno=ENOMEM; 
            fprintf(stderr, "errno value: %d\n", errno);  
            exit(EXIT_FAILURE); 
        }
        // Check if the file is regular 
        if(((isRegular (argv[i], NULL)) == 1)){
            memcpy(files[j], argv[i], length);
            j++;

        }
        
     
        
        
    }

     
    //  Set the arguments for master thread 
    master_args_t *master_args = (master_args_t *)malloc(sizeof(master_args_t));
    if(master_args == NULL){
        perror ("malloc fail main [farm]; master_args"); 
        errno=ENOMEM; 
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    master_args->file_list = linked_list_create();
    // Now add the file name parse from argv to the master_args->file_list
    for(int i = 0; i < number_of_file; ++i){
        linkedList_insert (master_args->file_list, master_args->file_list->length, (void*)files[i], (strlen(files[i]))*sizeof(char)+5);
    }
    
    //  Set the delay time between two request
    //  If delay time (opt -t) is not set, delay_request_time has value 0
    master_args->delay_request_time = delay_request_time; 
    
   

    //  If flag -d<dir> is set than get all the filename inside dir and his subdirectory 
    //  Get all the file name and add it to a linkedList (master_args->file_list)
    if(d_isSet == 1){
        // Check the argument
        struct stat statbuf;
        int r;
        SYSCALL_EXIT(stat,r,stat(source_dir, &statbuf),"Facendo stat del nome %s: errno=%d\n", source_dir, errno);
        
        if(!S_ISDIR(statbuf.st_mode)) {
            fprintf(stderr, "%s non e' una directory\n", source_dir);
            exit(EXIT_FAILURE);
        }    

        lsR(source_dir, master_args->file_list);
       
       
        
    }
    master_args->number_of_files = master_args->file_list->length; 
    number_of_file = master_args->file_list->length;
  


    //=================================================================================
    //  Queue creation 
        if(queue_len_set == 0){
            queue_len = 8; // If not specified as a command line option, the default value of the queue length is 8 
        }
        queue_t *queue = queue_create(queue_len);
        if(queue == NULL){
            fprintf(stderr,"Fail during queue creation");
            exit(EXIT_FAILURE); 
        }
        

    //=================================================================================
    //  thread creation and running 
        if(num_of_thread_set == 0){
            thread_num = 4; // If not specified as a command line option, the default number of thread is 4 (the same number of physical core on my machine)
        }
        thread_pool_t *thread_pool = thread_pool_create(thread_num, queue);
        if(thread_pool == NULL){
            exit(EXIT_FAILURE); 
        }
        pthread_t *master_thread = master_create(master_args, queue); 
        if(master_thread == NULL){
            exit(EXIT_FAILURE); 
        }

       
    //=================================================================================
    //  Pool, queue, task, and master thread destruction
        if ((wait(NULL)== -1)){
            perror("wait");
            fprintf(stderr, "Err code: %d", errno);  
            exit(EXIT_FAILURE); 
        }  
        if(sig_pipe_rise == 1){
            thread_pool_destroy (thread_pool); // Destroy the pool 
            master_thread_destroy(master_thread); // Destroy master_thread
            err = pthread_join(signal_handler, NULL); 
            if(err != 0){
                errno = err; 
                fprintf(stderr, "join signal thread Err code: %d\n", err); 
                exit(EXIT_FAILURE); 
            } 
         
           
            // Free all the data structure
            for (int i = 0; i < number_of_file_cpy_for_destruction; i ++){
                free(files[i]);
            }
            free(files);
            linked_list_destroy(master_args->file_list);
            queue_destroy(queue);
            free(master_args);

        
            // Remove the socket 
            if (remove("farm.sck") == -1) {
                perror("remove"); 
                fprintf(stderr, "Err code: %d", errno); 
                exit(EXIT_FAILURE);  
            }
            return -1; 
           
        }
        
        thread_pool_destroy (thread_pool); // Destroy the pool 
        master_thread_destroy(master_thread); // Destroy master_thread
        pthread_kill(signal_thread_id, SIGUSR2); // Send the signal to stop signal_handler_thread if no signal arises first
        err = pthread_join(signal_handler, NULL); 
        if(err != 0){
            errno = err; 
            fprintf(stderr, "join signal thread Err code: %d\n", err); 
            exit(EXIT_FAILURE); 
        } 
        
        
        // Free all the data structure
        for (int i = 0; i < number_of_file_cpy_for_destruction; i ++){
            free(files[i]);
        }
        free(files);
        linked_list_destroy(master_args->file_list);
        queue_destroy(queue);
        free(master_args);

    
        // Remove the socket 
        if (remove("farm.sck") == -1) {
            perror("remove"); 
            fprintf(stderr, "Err code: %d", errno); 
            exit(EXIT_FAILURE); 
        }
        
           
    }
    
    return 0; 
    
    
}
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
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <stdatomic.h>
#include <sys/wait.h>
#include "../include/linkedList.h"
#include "../include/queue.h"
#include "../include/thread_pool.h"
#include "../include/master_thread.h"
#include "../include/utilities.h"
#include "../include/util.h"

#if !defined(MAXFILENAME)
#define MAXFILENAME 2048
#endif
#define SOCKMAXPATH 108
#define BUFSZ 8
#define BUFSIZE 256
#define MAXCONN 100

volatile sig_atomic_t partial_result; 
volatile sig_atomic_t no_more_task; 
volatile sig_atomic_t sig_pipe_rise; 

pthread_t signal_thread_id = -1; 

//  FUNCTION PROTOTYPES

int isdot(const char dir[]);
void lsR(const char nomedir[], linkedList_t *dir_file_list) ;
static void *signal_handler_func (void *args);


//  Signal-handler function, it will be use by a thread to handle the signal 

static void *signal_handler_func (void *args){
    sigset_t* set = (sigset_t*) args; //    use to identify the signal set
    int signal; //  this is the signal recive

    signal_thread_id = pthread_self(); 
    //  infinite loop to handle the signal  
    while(1){
        sigwait(set, &signal); 
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
    SYSCALL_EXIT(stat,r,stat(nomedir,&statbuf),"Facendo stat del nome %s: errno=%d\n",
	    nomedir, errno);

    DIR * dir;
    
    if ((dir=opendir(nomedir)) == NULL) {
	perror("opendir");
	print_error("Errore aprendo la directory %s\n", nomedir);
	return;
    } else {
	struct dirent *file;
    
	while((errno=0, file =readdir(dir)) != NULL) {
	    struct stat statbuf;
	    char filename[MAXFILENAME]; 
	    int len1 = strlen(nomedir);
	    int len2 = strlen(file->d_name);
	    if ((len1+len2+2)>MAXFILENAME) {
		fprintf(stderr, "ERRORE: MAXFILENAME troppo piccolo\n");
		exit(EXIT_FAILURE);
	    }	    
	    strncpy(filename,nomedir,      MAXFILENAME-1);
	    strncat(filename,"/",          MAXFILENAME-1);
	    strncat(filename,file->d_name, MAXFILENAME-1);
	    
	    if (stat(filename, &statbuf)==-1) {
		perror("eseguendo la stat");
		print_error("Errore nel file %s\n", filename);
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
	if (errno != 0) perror("readdir");
	closedir(dir);
    }
}




int main (int argc, char *argv[]){

    //  Declaretion for handling the signals
//  Declaretion for handling the signals

    pthread_t signal_handler; // thread use to handle signals

    struct sigaction sig_act; 
    sigset_t sigset; 
    partial_result = 0; 
    no_more_task = 0; 

    sigemptyset(&sigset); 
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGUSR1);
    sigaddset(&sigset, SIGUSR2);
    sigaddset(&sigset, SIGPIPE);
    
    memset(&sig_act, 0, sizeof(sig_act));
    sig_act.sa_mask = sigset; 

    pthread_sigmask(SIG_BLOCK, &sigset, NULL); 

//  Create signal handler thread

    if( (pthread_create(&signal_handler, NULL, &signal_handler_func, (void*) &sigset)) != 0){
        perror("Signal handler thread creation"); 
        exit(EXIT_FAILURE); 
    } 

//=================================================================================
                                //COLLECTOR - child process 
//=================================================================================
//  Creation of the Collector process using fork() syscall 

    int process_id = fork(); 
    if(process_id == -1){
        perror("fork"); 
        return -1; 
    }

    //sleep(2); 
    //kill(getppid(), SIGPIPE); 
    
    // process_id = 0 means that we are in the child process (Collector)
   
    if (process_id == 0){
        //=========================================================================
        //  COLLECTOR PROCESS CODE
        
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
        bind(fd_sk, (struct sockaddr *)&sa, sizeof(sa)); 
        listen(fd_sk, SOMAXCONN); 

       
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
                       
                        FD_SET(fd_client, &set); 
                        if(fd_client > fd_num)
                            fd_num = fd_client; 
                        
                    }
                    else{
                        
                        n_byte_read = read(fd_index, buff, MAXFILENAME); 
                       
                        if(n_byte_read == 0){
                            FD_CLR(fd_index, &set);
					        if (fd_index == fd_num) fd_num--;
                            close(fd_index); 
                        }
                        else{
                            
                            //  Split the request string (request string = "<long integer> <filename.dat>")
                            char *token; 
                            token = strtok(buff, " ");  //  It splis on the space
                            

                            //  Convert the first string from char* to long 
                            long binary_result = 0;
                            if(isNumber(token, &binary_result) == 2){
                                perror("overflow/underflow"); 
                                errno = ERANGE; 
                                fprintf(stderr, "Err code: %d", errno); 
                                return 1;   
                            }
                            
                            //  Printing partial result (if SIGSUR1 arise)
                            if(strcmp(token, "print")== 0){
                                for(int j = 0; j < result_list->length; j++){
                                    entry = (node_t*)linkedList_access_data(result_list, j); 
                                    if(entry == NULL){
                                        errno = EINVAL; 
                                        perror("main [farm] linked_list_access_data"); 
                                        fprintf(stderr, "Err code: %d\n", errno); 
                                        return 1;
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

    int thread_num; 
    int queue_len; 
    int delay_request_time = 0; 
    char source_dir[FILENAME_MAX]; 
    int d_isSet = 0; 
    int num_of_thread_set = 0; 
    int queue_len_set = 0; 
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
                sscanf(optarg, "%d", &thread_num);
                cont_opt ++;
                num_of_thread_set = 1; 
                //printf("caso -n %d\n", thread_num); 
                break;
            // This option specifies the upper bound of the queue 
            case 'q':
                // I use sscanf to read from optarg the queue's upper bound and I write it in queue_len 
                sscanf(optarg, "%d", &queue_len);
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
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    for(int i = cont_opt + 1; i < argc; ++i){
        size_t length = strlen(argv[i])+1;
        files[j] = malloc(length * (sizeof(char*)));
        if(files[j] == NULL){
            perror ("malloc fail main [farm]; files[j]"); 
            errno = ENOMEM;
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
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    master_args->file_list = linked_list_create();
    // Now add the file name parse from argv to the master_args->file_list
    for(int i = 0; i < number_of_file; ++i){
        linkedList_insert (master_args->file_list, master_args->file_list->length, (void*)files[i], (strlen(files[i]))*sizeof(char)+5);
    }
    
    //  Set the delay time between two request
    master_args->delay_request_time = delay_request_time; 
    
   

    //  If flag -d<dir> is set than get all the filename inside dir and his subdirectory 
    //  Get all the file name and add it to a linkedList (master_args->file_list)
    if(d_isSet == 1){
        // Check the argument
        struct stat statbuf;
        int r;
        SYSCALL_EXIT(stat,r,stat(source_dir, &statbuf),
            "Facendo stat del nome %s: errno=%d\n",
            source_dir, errno);
        if(!S_ISDIR(statbuf.st_mode)) {
            fprintf(stderr, "%s non e' una directory\n", source_dir);
            return EXIT_FAILURE;
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
        

    //=================================================================================
    //  thread creation and running 
        if(num_of_thread_set == 0){
            thread_num = 4; // If not specified as a command line option, the default number of thread is 4 (the same number of physical core on my machine)
        }
        thread_pool_t *thread_pool = thread_pool_create(thread_num, queue);
        pthread_t *master_thread = master_create(master_args, queue); 


        //sleep(2); 
        //kill(getppid(), SIGPIPE); 
    //=================================================================================
    //  Pool, queue, task, and master thread destruction
        wait(NULL); 
        if(sig_pipe_rise == 1){
            fprintf(stderr, "elimino strutture dati causa sigpipe\n");
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
                return errno; 
            }
            fprintf(stderr, "sopra la exit causa sigpipe master\n");
            exit(EXIT_FAILURE); 
           
        }
        
        thread_pool_destroy (thread_pool); // Destroy the pool 
        master_thread_destroy(master_thread); // Destroy master_thread
        pthread_kill(signal_thread_id, SIGUSR2); // Send the signal to stop signal_handler_thread if no signal arises first
        pthread_join(signal_handler, NULL); 
        
        
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
            return 1; 
        }
        
           
    }
    
    return 0; 
    
    
}
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include<string.h>
#include <unistd.h>
#include <time.h>
#include "../include/linkedList.h"
#include "../include/node.h"


// FUNCTION PROTOTYPES

node_t * linkedList_create_node(void *data, long value, unsigned long size);
void linkedList_destroy_node(node_t *node_to_destroy);
node_t * iterate_on_list(linkedList_t *linked_list, int index);
void *linkedList_insert(linkedList_t *linked_list, int index, void *data, unsigned long size);
void *linkedList_remove_node(linkedList_t *linked_list, int index);
void * linkedList_access_data(linkedList_t *linked_list, int index);
void *order_insertion (linkedList_t *linked_list, long value, char *path);


linkedList_t *linked_list_create(){
    linkedList_t *new_list = (linkedList_t *) malloc(sizeof(linkedList_t));
    if(new_list == NULL){
        perror ("malloc fail linked_list_create;"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    new_list->head = NULL; 
    new_list->tail = NULL; 
    new_list->length = 0;
    
    return new_list;
}

void linked_list_destroy(linkedList_t *linked_list){
    if(linked_list == NULL){
        return; 
    }
    node_t *tmp; 
    for (int i = 0; i < linked_list->length; i++)
    {
        tmp = linked_list->head; 
        linked_list->head = linked_list->head->next;
        destroy_node(tmp); 
    }
    free(linked_list);
}




// The create_node function creates a new node to add to the list by allocating space on the heap and calling the node constructor.
node_t * linkedList_create_node(void *data, long value, unsigned long size){
    // Call the constructor for the node
    return  create_node(data, value, size);
     
}

// The destroy_node function removes a node by deallocating it's memory address.  This simply renames the node destructor function.
void linkedList_destroy_node(node_t *node_to_destroy){
    destroy_node(node_to_destroy);
}

// The iterate function traverses the list from beginning to end.
node_t * iterate_on_list(linkedList_t *linked_list, int index){

    if(linked_list == NULL){
        errno = EINVAL;
        perror("linkedList: iterate_on_list"); 
        fprintf(stderr, "Need to pass a pointer to a LinkedList type object Err code: %d\n", errno); 
        exit(EXIT_FAILURE); 
    }


    // Check if is specified a valid index.
    if (index < 0 || index >= linked_list->length)
    {
        return NULL;
    }
    // Create a current node for iteration.
    node_t *current = linked_list->head;
    // Step through the list until the desired index is reached.
    for (int i = 0; i < index; i++){
        current = current->next;
    }
    return current;
}


// The insert a new node, sorted in ascending order, in the list.
// An auxiliary index is used to simplify the management of the linkedList.
void *linkedList_insert_order(linkedList_t *linked_list, int index, void *data, long value, unsigned long size){
    
    if(data == NULL){
        errno = EINVAL; 
        perror("linkedList: linkedList_insert_order"); 
        fprintf(stderr, "data can't be NULL. Err code: %d\n", errno); 
        return NULL; 
    }

    if(linked_list == NULL){
        errno = EINVAL;
        perror("linkedList: linkedList_insert_order"); 
        fprintf(stderr, "Need to pass a pointer to a LinkedList type object Err code: %d\n", errno); 
        return NULL; 
    }
    if(size == 0){
        errno = EINVAL;
        perror("linkedList: linkedList_insert_order"); 
        fprintf(stderr, "Size can't be zero %d\n", errno); 
        return NULL; 
    }

    
    // Create a new node
   
    node_t *node_to_insert = linkedList_create_node(data, value, size);
    // Check if this node will be the new head of the list.
    if (index == 0){
       
        if(linked_list->head == NULL){
            node_to_insert->next = linked_list->head;
            
            // Redefine the list's head and set the tail
            linked_list->head = node_to_insert;
            linked_list->tail = node_to_insert; 

        }
      
        node_to_insert->next = linked_list->head;
       
      
        // Redefine the list's head
        linked_list->head = node_to_insert;
    
       
        
    }else if (index == linked_list->length && index != 0){
        
        // Redefine the list's tail
        node_to_insert->next = NULL;
        linked_list->tail->next = node_to_insert;
        linked_list->tail = node_to_insert;
       
        
    }
    else{
         
        // Find the item in the list immediately before the desired index.
        node_t *current = iterate_on_list(linked_list, index - 1);
        // Set the Node's next.
        node_to_insert->next = current->next;
        // Set the current's next to the new node.
        current->next = node_to_insert;
       
        
    }
   
    // Increment the list length.
    linked_list->length += 1;
    return 0; // Use to remove the Warning: control reaches end of non-void function 

}

//  insert the data of size "size" in the given linked_list at the specified index
//  in case of failure it will return null
void *linkedList_insert(linkedList_t *linked_list, int index, void *data, unsigned long size){
    if(data == NULL){
        errno = EINVAL;
        perror("linkedList: linkedList_insert");  
        fprintf(stderr, "data can't be NULL. Err code: %d\n", errno); 
        return NULL; 
    }

    if(linked_list == NULL){
        errno = EINVAL;
        perror("linkedList: linkedList_insert"); 
        fprintf(stderr, "Need to pass a pointer to a LinkedList type object Err code: %d\n", errno); 
        return NULL; 
    }

    if(size == 0){ 
        errno = EINVAL;
        perror("linkedList: linkedList_insert");  
        fprintf(stderr, "Size can't be zero %d\n", errno); 
        return NULL; 
    }
    
    
    // Create a new node
    node_t *node_to_insert = linkedList_create_node(data, 0, size); // The "value" fild is set to 0 because i use this function only to 
                                                                    // insert filename in the queue and in master_args struct, so i don't need any  value. 
                                                                    // the value will be calculate by the threadpool and the fild will be set by collector process 
    // Check if this node will be the new head of the list.
    if (index == 0){
        node_to_insert->next = linked_list->head;
        // Redefine the list's head
        linked_list->head = node_to_insert;
    }
    else{
        // Find the item in the list immediately before the desired index.
        node_t *current = iterate_on_list(linked_list, index - 1);
        // Set the Node's next.
        node_to_insert->next = current->next;
        // Set the current's next to the new node.
        current->next = node_to_insert;
        
    }
    // Increment the list length.
    linked_list->length += 1;
    return 0; // Use to remove the Warning: control reaches end of non-void function 
}

// The remove function removes a node from the linked list.
void *linkedList_remove_node(linkedList_t *linked_list, int index){

    if(linked_list == NULL){
        errno = EINVAL; 
        perror("linkedList: linkedList_remove_node"); 
        fprintf(stderr, "Need to pass a pointer to a LinkedList type object Err code: %d\n", errno); 
        return NULL; 
    }


    // Check if the item to removed is the head.
    if (index == 0){
        node_t *node_to_remove = linked_list->head;
        // Define the new head for the list.
        if (node_to_remove){
            linked_list->head = node_to_remove->next;
            // Remove the node.
            linkedList_destroy_node(node_to_remove);
        }
    }
    else{
        // Find the item in the list before the one that is going to be removed.
        node_t *current = iterate_on_list(linked_list, index - 1);
        // Use the current to define the node to be removed.
        node_t *node_to_remove = current->next;
        // Update the current's next to skip the node to be removed.
        current->next = node_to_remove->next;
        // Remove the node.
        linkedList_destroy_node(node_to_remove);
    }
    // Decrement the list length.
    linked_list->length -= 1;
    return 0; // Use to remove the Warning: control reaches end of non-void function
}

// The linkedList_access_data function is used to access data in the list.
void * linkedList_access_data(linkedList_t *linked_list, int index){

    if(linked_list == NULL){
        errno = EINVAL;
        perror("linkedList: linkedList_access_data");  
        fprintf(stderr, "Need to pass a pointer to a LinkedList type object Err code: %d\n", errno); 
        return NULL; 
    }



    // Find the desired node and return its data.
    node_t *current = iterate_on_list(linked_list, index);
    if (current){
        return (node_t*) current;
    }
    else{
        return NULL;
    }
}

//  Insert entry in a linked list by ascending order
void *order_insertion (linkedList_t *linked_list, long value, char *path){
    if(path == NULL){
        errno = EINVAL; 
        perror("linkedList order_insertion"); 
        fprintf(stderr, "path can't be NULL. Err code: %d\n", errno); 
        return NULL; 

    }
    if(linked_list == NULL){
        errno = EINVAL;
        perror("linkedList order_insertion"); 
        fprintf(stderr, "Need to pass a pointer to a LinkedList type object. Err code: %d\n", errno); 
        return NULL; 

    }
    node_t *current = linked_list->head;
    if(current == NULL || linked_list->head->value >= value){ // Head insertion 
        if((linkedList_insert_order(linked_list, 0, (void*)path, value, ((strlen(path))*sizeof(char)+5)) == NULL)){
            return NULL;
        }
    } 
    else if(linked_list->tail->value <= value){ // Tail insertion 
        if((linkedList_insert_order(linked_list, linked_list->length, (void*)path, value, ((strlen(path))*sizeof(char)+5)) == NULL)){
            return NULL; 
        } 
        
    }
    else{ // Insertion in middle 
        int list_length = linked_list->length; // use 'cause linked_list->length is chainging during insertion, so the for loop become an infinite loop cause i never catch linked_list->length
        for(int i = 0; i < list_length; i++){
            if(current->value >= value){
                if((linkedList_insert_order(linked_list, i, (void*)path, value, ((strlen(path))*sizeof(char)+5)) == NULL)){
                    return NULL; 
                } 
                break;
            }
            current = current->next; 
        }

    }
    return 0; // Use to remove the Warning: control reaches end of non-void function

}



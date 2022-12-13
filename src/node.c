#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "../include/node.h"

// FUNCTION PROTOTYPES
node_t *create_node(void *data, long value, unsigned long size);
void destroy_node(node_t *node);

// The constructor is used to create new instances of nodes.
node_t *create_node(void *data, long value, unsigned long size){
    if (size < 1)
    {
        // Confirm the size of the data is at least one, otherwise exit with an error message.
        printf("Invalid data size for node...\n");
        exit(1);
    }
    // Create a Node instance to be returned
    node_t *node = (node_t*)malloc(sizeof(node_t));
    if(node == NULL){
        perror ("malloc fail create_node; node"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    // Allocate space for the data if it is of a supported type
    node->data = (void*) malloc(size + 1);
    if(node->data == NULL){
        perror ("malloc fail create_node; node->data"); 
        errno = ENOMEM;
        fprintf(stderr, "errno value: %d\n", errno);  
        exit(EXIT_FAILURE); 
    }
    memcpy(node->data, data, size);
    node->value = value; 
    // Initialize the pointers.
    node->next = NULL;
    node->previous = NULL;
    return node;
}

// Removes a node by freeing the node's data and its node.
void destroy_node(node_t *node){
    free(node->data);
    free(node);
}


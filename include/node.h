
#ifndef node_h
#define node_h


// Nodes are used to store data of any type in a list.  
typedef struct node{
    // The data is stored as a void pointer - casting is required for proper access.
    void *data;
    long value; 
    // A pointer to the next node in the chain.
    struct node *next;
    struct node *previous;
}node_t;




// The constructor should be used to create nodes.
node_t *create_node(void *data, long value, unsigned long size);
// The destructor should be used to destroy nodes.
void destroy_node(node_t *node);

#endif /* node_h */

#ifndef task_h
#define task_h
//
// Created by lorenzo on 26/9/22.
//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


// DATA TYPES


typedef struct task{
    // The function to be executed.
    void * (*taskfunc)(char*);
    // The argument to be passed.
    char *arg;
}task_t;


#endif 
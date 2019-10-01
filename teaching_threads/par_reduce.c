/**
 * Teaching Threads
 * CS 241 - Fall 2019
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "reduce.h"
#include "reducers.h"

/* You might need a struct for each task ... */
typedef struct task {
    pthread_t id;
    reducer r_func;
    int base_case;
    int * list;
    size_t list_len;
    int result;

} task;
/* You should create a start routine for your threads. */
void sub_reduce(task * data) {
    if (!data) return;

    data->result = reduce(data->list, data->list_len, data->r_func, data->base_case);
}
int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    task * tasks = calloc(num_threads, sizeof(task));

    free(tasks);
    
    return 0;
}

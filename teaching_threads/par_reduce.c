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
void * sub_reduce(task * data) {
    if (!data) return NULL;

    data->result = reduce(data->list, data->list_len, data->r_func, data->base_case);
    return NULL;
}

void initialize_tasks(task * t, reducer r_func, int base_case, 
                        int * list, size_t len, size_t num_threads) {
    
    size_t default_sub_len = len / num_threads;
    
    size_t idx = 0;
    for(; idx < num_threads; idx++) {
        size_t offset = (default_sub_len * idx);
        size_t sub_len = (idx < num_threads - 1) ? default_sub_len : len - offset;
        
        t[idx].list = list + offset;
        t[idx].list_len = sub_len;
        t[idx].r_func = r_func;
        t[idx].base_case = base_case;
        printf("first: %d\n", t[idx].list[0]);
        
        
    }
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    task * tasks = calloc(num_threads, sizeof(task));
    initialize_tasks(tasks, reduce_func, base_case, list, list_len, num_threads);
   
   
    size_t thread_idx = 0;    
    for (; thread_idx < num_threads; thread_idx++) {
        pthread_create(&tasks[thread_idx].id, NULL, (void * ) sub_reduce, &tasks[thread_idx]);
    }
    
    int result[num_threads];
    size_t result_idx = 0;

    for(; result_idx < num_threads; result_idx++) {
        pthread_join(tasks[result_idx].id, NULL);
        result[result_idx] = tasks[result_idx].result;
    }

    int final_result = reduce(result, num_threads, reduce_func, 0);
    free(tasks);
    
    return final_result;
}

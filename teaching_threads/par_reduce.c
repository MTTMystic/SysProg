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

void initialize_task(task * t, reducer r_func, int base_case, int * list, size_t len) {
    t->r_func = r_func;
    t->base_case = base_case;
    t->list = list;
    t->list_len = len;
}

int par_reduce(int *list, size_t list_len, reducer reduce_func, int base_case,
               size_t num_threads) {
    /* Your implementation goes here */
    task * tasks = calloc(num_threads, sizeof(task));
    size_t thread_idx = 0;
    size_t default_sub_len = list_len / num_threads;
    for (; thread_idx < num_threads; thread_idx++) {
        size_t offset_idx = default_sub_len * thread_idx;
        size_t sub_len = default_sub_len;
        if (thread_idx == num_threads - 1) {
            sub_len = list_len - (default_sub_len * thread_idx);
        }

        initialize_task(&tasks[thread_idx], reduce_func, base_case, list + offset_idx, sub_len);
        pthread_create(&tasks[thread_idx].id, NULL, (void * ) sub_reduce, &tasks[thread_idx]);
    }
    free(tasks);
    
    return 0;
}

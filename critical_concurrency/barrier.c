/**
 * Critical Concurrency
 * CS 241 - Fall 2019
 */
#include "barrier.h"

// The returns are just for errors if you want to check for them.
int barrier_destroy(barrier_t *barrier) {
    int error = 0;
    pthread_mutex_destroy(&barrier->mtx);
    pthread_cond_destroy(&barrier->cv);
    return error;
}

int barrier_init(barrier_t *barrier, unsigned int num_threads) {
    pthread_mutex_init(&barrier->mtx, NULL);
    pthread_cond_init(&barrier->cv, NULL);

    barrier->n_threads = num_threads;
    barrier->count = 0;
    barrier->times_used = 1;
    
    int error = 0;
    return error;
}

int barrier_wait(barrier_t *barrier) {
    //lock mutex
    pthread_mutex_lock(&barrier->mtx);

    //using time_used as binary active/inactive state
    while (!barrier->times_used) {
        //if barrier is not active (times_used == 0) do nothing until it is 
        //do not allow new use of barrier (i.e. next iteration) until all threads left
        pthread_cond_wait(&barrier->cv, &barrier->mtx);
    }

    //increment count (# of threads that called this function)
    barrier->count++;

    //handle case where barrier threshold met (first thread to leave barrier)
    if (barrier->count == barrier->n_threads) {
        //mark the barrier as inactive so other waiting threads can leave
        barrier->times_used = 0;

        //decrease count
        //do not reset count because other threads not "awake" yet
        barrier->count--;

        //awaken waiting threads (waiting on inactive barrier)
        pthread_cond_broadcast(&barrier->cv);
    } else {
        //threads must wait until barrier is deactivated (threshold hit, see above)
        while (barrier->times_used) {
            pthread_cond_wait(&barrier->cv, &barrier->mtx);
        }

        //after awaking, other threads can leave barrier
        barrier->count--;

        //last thread to leave barrier needs to mark it as active again (for reusability)
        if (!barrier->count) {
            barrier->times_used = 1;
             //awaken waiting threads (waiting for active barrier)
            pthread_cond_broadcast(&barrier->cv);
        }

       
    }

    //unlock mutex
    pthread_mutex_unlock(&barrier->mtx);
    return 0;
}

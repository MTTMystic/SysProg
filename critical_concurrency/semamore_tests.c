/**
 * Critical Concurrency
 * CS 241 - Fall 2019
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "semamore.h"

static Semamore * semm_5;
void * semm_post_loop() {
    int post_iter = 0;
    for (; post_iter < 10; post_iter++) {
        semm_post(semm_5);
    }

    return NULL;
}

void * semm_wait_loop() {
    int wait_iter = 0;
    for (; wait_iter < 10; wait_iter++) {
        semm_wait(semm_5);
    }

    return NULL;
}


int main(int argc, char **argv) {
    //printf("Please write tests in semamore_tests.c\n");
    semm_5 = malloc(sizeof(Semamore));
    semm_init(semm_5, 0, 3);
    pthread_t post_thread, wait_thread;

    pthread_create(&post_thread, NULL, semm_post_loop, NULL);
    pthread_create(&wait_thread, NULL, semm_wait_loop, NULL);

    pthread_join(post_thread, NULL);
    pthread_join(wait_thread, NULL);

    semm_destroy(semm_5);

    free(semm_5);

    return 0;
}

/**
 * Deadlock Demolition
 * CS 241 - Fall 2019
 */
#include "libdrm.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static drm_t * drm0;
static drm_t * drm1;

void lock_unlock_test (void * tid) {
    pthread_t * t_id = (pthread_t *) tid;
    
    drm_t * test_drm = drm_init();

  
    printf("now waiting on drm....\n");
    int wait_result = drm_wait(test_drm, t_id);
    assert(wait_result);
    printf("drm is locked\n");
    int post_result = drm_post(test_drm, t_id);
    assert(post_result);
    printf("drm is unlocked\n");
    drm_destroy(test_drm);
}

void concurrent_lock_test(void * tid) {
    
    pthread_t * t_id = (pthread_t *) tid;
    printf("Thread %d waiting to lock drm.\n", *(int *)t_id);
    drm_wait(drm0, t_id);
    printf("Thread %d has locked drm\n", *(int *)t_id);
    sleep(2);
    drm_post(drm0, t_id);
    printf("Thread %d unlocked drm\n", *(int *)t_id);
}

void double_lock_test(void * tid) {
    pthread_t * t_id = (pthread_t *) tid;
    drm_t * test_drm = drm_init();
    
    printf("Thread %d tries to lock drm.\n", *(int *)t_id);
    int first_wait = drm_wait(test_drm, t_id);
    assert(first_wait == 1);
    printf("Thread %d tries to lock drm again.\n", *(int *)t_id);
    int double_wait = drm_wait(test_drm, t_id);
    assert(double_wait == 0);
    printf("Thread %d unlocks drm\n", *(int *)t_id);
    drm_post(test_drm, t_id);

    drm_destroy(test_drm);
}

void deadlock_test_p1(void * tid) {
    pthread_t * t_id = (pthread_t *) tid;

    //lock drm0
    printf("p1 trying to lock drm0\n");
    int wait = drm_wait(drm0, t_id);
    assert(wait);
    //give the other thread time to start up
    sleep(2);
    printf("p1 trying to lock drm1\n");
    //try to lock drm1
    int p1_r2_result = drm_wait(drm1, t_id);
    printf("result of p1 trying to lock r2 was: %d\n", p1_r2_result);
    printf("trying to unlock r1\n");
    drm_post(drm0, t_id);
    printf("unlocked r1\n");
}   

void deadlock_test_p2(void * tid) {
    pthread_t * t_id = (pthread_t *) tid;

    //lock drm1
    printf("p2 trying to lock drm1\n");
    int wait = drm_wait(drm1, t_id);
    assert(wait);
    //give the other thread time to start up
    sleep(2);
    //try to lock drm0
    printf("p2 trying to lock drm0\n");
    int p2_r1_result = drm_wait(drm0, t_id);
    printf("result of p2 trying to lock r1 was: %d\n", p2_r1_result);
    printf("trying to unlock r2\n");
    drm_post(drm1, t_id);
    printf("unlocked r2\n");
}

int main() {
    
    printf("----- basic lock/unlock -----\n");

    pthread_t test_thread0;
    pthread_create(&test_thread0, NULL, (void *) lock_unlock_test, &test_thread0);
    pthread_join(test_thread0, NULL);

    drm0 = drm_init();
    drm1 = drm_init(); 

    printf("----- concurrent lock -----\n");
    
    pthread_t test_thread1;
    pthread_t test_thread2;

    pthread_create(&test_thread1, NULL, (void *) concurrent_lock_test, &test_thread1);
    pthread_create(&test_thread2, NULL, (void *) concurrent_lock_test, &test_thread2);
    pthread_join(test_thread1, NULL);
    pthread_join(test_thread2, NULL);

    printf("----- double lock -----\n");

    pthread_t test_thread3;
    pthread_create(&test_thread3, NULL, (void *) double_lock_test, &test_thread3);
    pthread_join(test_thread3, NULL);

    
    
    printf("----- deadlock prevention -----\n");

    pthread_t test_thread4;
    pthread_t test_thread5;

    pthread_create(&test_thread4, NULL, (void *) deadlock_test_p1, &test_thread4);
    pthread_create(&test_thread5, NULL, (void *) deadlock_test_p2, &test_thread5);
    pthread_join(test_thread4, NULL);
    pthread_join(test_thread5, NULL);
    
    return 0;
}

/**
 * Critical Concurrency
 * CS 241 - Fall 2019
 */
#include "queue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This queue is implemented with a linked list of queue_nodes.
 */
typedef struct queue_node {
    void *data;
    struct queue_node *next;
} queue_node;

struct queue {
    /* queue_node pointers to the head and tail of the queue */
    queue_node *head, *tail;

    /* The number of elements in the queue */
    ssize_t size;

    /**
     * The maximum number of elements the queue can hold.
     * max_size is non-positive if the queue does not have a max size.
     */
    ssize_t max_size;

    /* Mutex and Condition Variable for thread-safety */
    pthread_cond_t cv;
    pthread_mutex_t m;
};

queue *queue_create(ssize_t max_size) {
    queue * new_q = malloc(sizeof(queue));
    new_q->head = NULL;
    new_q->tail = NULL;

    new_q->size = 0;
    new_q->max_size = max_size;

    pthread_cond_init(&new_q->cv, NULL);
    pthread_mutex_init(&new_q->m, NULL);

    return new_q;
}

void queue_destroy(queue *this) {
    pthread_cond_destroy(&this->cv);
    pthread_mutex_destroy(&this->m);

    queue_node * iter = this->head;
    while(iter) {
        queue_node * next = iter->next;
        free(iter);
        iter = next;
    }

    free(this);
}

void queue_push(queue *this, void *data) {
    //lock mutex to prevent concurrent pushes
    pthread_mutex_lock(&this->m);

    //force threads to wait if there is a max size and q is at capacity
    while(this->max_size > 0 && this->size == this->max_size) {
        pthread_cond_wait(&this->cv, &this->m);
    }

    //linked list insertion at tail
    queue_node * new_node = malloc(sizeof(queue_node));
    new_node->data = data;
    new_node->next = NULL;
    //handle case where queue is empty
    if (this->size == 0) {
        this->head = new_node;
        this->tail = new_node;
    } else {
        this->tail->next = new_node; //mark new_node as tail
        this->tail = new_node;
    }

    //update size variable
    this->size++;

    if (this->size == 1) {
        //awaken a thread waiting to pull from list, now non-empty
        pthread_cond_signal(&this->cv);
    }


    //unlock the mutex
    pthread_mutex_unlock(&this->m);
}

void *queue_pull(queue *this) {
    /* Your code here */
    //lock mutex to prevent concurrent pulls
    pthread_mutex_lock(&this->m);

    //force threads to wait if q is empty
    while (this->size == 0) {
        pthread_cond_wait(&this->cv, &this->m);
    }

    //linked list removal at head
    //list is guaranteed to have at least 1 elem by now, so head is not NULL

    void * pop_data = this->head->data;
    this->head = this->head->next;
    this->size--;

    //handle case where removal made list empty (update tail)
    if (this->size == 0) {
        this->tail = NULL;
    }

    //signal threads waiting to push to q
    pthread_cond_signal(&this->cv);

    //unlock mutex...
    pthread_mutex_unlock(&this->m);

    return pop_data;
}

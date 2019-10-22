/**
 * Password Cracker
 * CS 241 - Fall 2019
 */
#include "cracker2.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

#include <unistd.h>
#include <crypt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct _task_t {
    char * username; //maximum 8 chars + null terminator
    char * hash; //maximum 13 chars + null terminator
    char * password; //same length rule as u/n
    int known_chars; //indicates length of known prefix
} task_t;

typedef struct _subtask_t {
    int idx; //idx of this subtask thread
    long start_pos; //starting password possibility
    long end_pos; //ending password possibility
    task_t * task;
    char * password; //password for this subtask
    long attempts; //hashing attempts made so far
    int status; //current state of subtask thread
    double elapsed_time; //time elapsed for this subtask
} subtask_t;

typedef struct _worker_t {
    pthread_t tid;
    int idx;
} worker_t;

//global # of threads for convenience
static size_t _tcount;

//has password been found? reset for each task
static char pw_found;

//lock for modifying pw_found
static pthread_mutex_t flag_lock = PTHREAD_MUTEX_INITIALIZER;
//lock for modifying subtask info
static pthread_mutex_t st_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t start_lock = PTHREAD_MUTEX_INITIALIZER;

//barrier for task sync
static pthread_barrier_t task_barrier;

static pthread_barrier_t st_barrier;

static pthread_cond_t start_cv;

static char start_processing;

static queue * task_queue;

static subtask_t * subtasks;

static worker_t * workers;

task_t * string_to_task() {
    char * buffer = NULL;
    size_t buffer_len = 0;
    ssize_t bytes_read = 0;

    while ((bytes_read = getline(&buffer, &buffer_len, stdin)) != -1) {
        buffer[bytes_read - 1] = '\0';

        char * un = buffer;

        char * hash = strchr(un, ' ');
        hash[0] = '\0';
        hash++;

        char * pw = strchr(hash, ' ');
        pw[0] = '\0';
        pw++;

        task_t * pw_task= malloc(sizeof(task_t));

        pw_task->username = un;
        pw_task->hash = hash;
        pw_task->password = pw;

        pw_task->known_chars = getPrefixLength(pw_task->password);
        
        return pw_task;
    }

    free(buffer);
    return NULL;
};

void process_subtask(void * subtask);

void create_workers() {
    size_t t_idx = 0;
    for (; t_idx < _tcount; t_idx++) {
        printf("Creating a worker.\n");
        workers[t_idx].idx = t_idx + 1;
        pthread_create(&workers[t_idx].tid, NULL, (void *) process_subtask, &workers[t_idx].idx);
         printf("Finished creating a worker.\n");
    }
}

void join_workers() {
    size_t t_idx = 0;
    for (; t_idx < _tcount; t_idx++) {
        pthread_join(workers[t_idx].tid, NULL);
    }
}

void read_tasks() {
    //max strlen for line: 8(u/n) + 1(space) + 13(hash) + 1(space) + 8(known chars) + 1(\0) 
   task_t * new_task;
    while((new_task = string_to_task()))
     {
        queue_push(task_queue, new_task);
    }
    task_t * empty_task = (task_t *) malloc(sizeof(task_t));
    empty_task->username = "emp_task";
    queue_push(task_queue, empty_task);
}

void initialize_subtask(size_t st_idx, task_t * task) {
    //how many chars are unknown?
    size_t unknown_chars = strlen(task->password) - task->known_chars;
    
    long start_pos;
    long end_pos;

    //get subrange that this subtask covers
    getSubrange(unknown_chars, _tcount, st_idx + 1, &start_pos, &end_pos);
    
    subtask_t * subtask = &subtasks[st_idx];

    subtask->idx = st_idx + 1;
    subtask->task = task;
    subtask->start_pos = start_pos;
    subtask->end_pos = end_pos;
    subtask->attempts = 0;
    subtask->elapsed_time = 0;
    subtask->status = 0;
    subtask->password = strdup(task->password);
    setStringPosition(subtask->password + subtask->task->known_chars, start_pos);
}

void process_subtask(void * idx_ptr) {
    int * idx_intptr = (int *) idx_ptr;
    int subtask_idx = (*idx_intptr) - 1;

    subtask_t * st = &subtasks[subtask_idx];

    while (!start_processing) {
        pthread_cond_wait(&start_cv, &start_lock);
    }
    pthread_mutex_unlock(&start_lock);

    while (1) {
        pthread_mutex_lock(&st_lock); 
        if (!strcmp(st->task->username, "emp_task")) {
            break;
        }
        pthread_mutex_unlock(&st_lock);

        

        double start_time = getThreadCPUTime();

        pthread_barrier_wait(&st_barrier);
        pthread_mutex_lock(&st_lock); 
        v2_print_thread_start(st->idx, st->task->username, st->start_pos, st->password);
        pthread_mutex_unlock(&st_lock);
        struct crypt_data cdata;
        cdata.initialized = 0;
        char * hash;

        char not_found = 1;

        for (st->attempts = 1; st->attempts <= st->end_pos; st->attempts++) {
            hash = crypt_r(st->password, "xx", &cdata);

            if (!strcmp(hash, st->task->hash)) {
                st->status = 1;
                pthread_mutex_lock(&flag_lock);
                pw_found = 1;
                pthread_mutex_unlock(&flag_lock);

                pthread_mutex_lock(&st_lock);
                v2_print_thread_result(st->idx, st->attempts, 0);
                pthread_mutex_unlock(&st_lock);
                st->elapsed_time = getThreadCPUTime() - start_time;
                not_found = 0;
                pthread_barrier_wait(&task_barrier);
                break;
            }

            incrementString(st->password);

            pthread_mutex_lock(&flag_lock);
            char found = pw_found;
            pthread_mutex_unlock(&flag_lock);

            if (found) {
                st->status = 2;
                pthread_mutex_lock(&st_lock);
                v2_print_thread_result(st->idx, st->attempts, 1);
                pthread_mutex_unlock(&st_lock);
                st->elapsed_time = getThreadCPUTime() - start_time;
                not_found = 0;
                pthread_barrier_wait(&task_barrier);
                break;
            }
        }

        if (not_found) {
            st->status = 3;
            pthread_mutex_lock(&st_lock);
            v2_print_thread_result(st->idx, st->attempts, 2);
            pthread_mutex_unlock(&st_lock);
            st->elapsed_time = getThreadCPUTime() - start_time;
            pthread_barrier_wait(&task_barrier);
        }
        

    }
}

void task_loop() {
    task_t * task;
    while ((task = queue_pull(task_queue))) {
        
        if (!strcmp(task->username, "emp_task")) {
            size_t t_idx = 0;
            for (; t_idx < _tcount; t_idx++) {
                pthread_mutex_lock(&st_lock);
                subtasks[t_idx].task->username = "emp_task";
                pthread_mutex_unlock(&st_lock);
            }

            return;
        }

        pw_found = 0;
        double start_time = getTime();


        v2_print_start_user(task->username);
        
        size_t t_idx = 0;

        for (; t_idx < _tcount; t_idx++) {
            pthread_mutex_lock(&st_lock);
            initialize_subtask(t_idx, task);
            pthread_mutex_unlock(&st_lock);
        }

        if (!start_processing) {
            pthread_mutex_lock(&start_lock);
            start_processing = 1;
            pthread_cond_broadcast(&start_cv);
            pthread_mutex_unlock(&start_lock);
        }
        
        pthread_barrier_wait(&task_barrier);

        t_idx = 0;
        char failed = 1;
        char * correct_pw = NULL;
        long total_attempts = 0;
        double total_cpu_time = 0;

        for (; t_idx < _tcount; t_idx++) {
           if (subtasks[t_idx].status == 1) {
               failed = 0;
               correct_pw = subtasks[t_idx].password;
           }

           total_attempts += subtasks[t_idx].attempts;
           total_cpu_time += subtasks[t_idx].elapsed_time;
        }

        v2_print_summary(subtasks[0].task->username, correct_pw, total_attempts, getTime() - start_time, total_cpu_time, failed);
    }
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads
    _tcount = thread_count;
    pthread_barrier_init(&task_barrier, NULL, thread_count + 1);
     pthread_barrier_init(&st_barrier, NULL, thread_count);
    pthread_cond_init(&start_cv, NULL);

    subtasks = calloc(thread_count, sizeof(subtask_t));
    workers = calloc(thread_count, sizeof(worker_t));
    task_queue = queue_create(0);

    create_workers();

    read_tasks();

    task_loop();

    join_workers();

    pthread_mutex_destroy(&flag_lock);
    pthread_mutex_destroy(&st_lock);

    pthread_barrier_destroy(&task_barrier);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}

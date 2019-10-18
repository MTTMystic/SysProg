/**
 * Password Cracker
 * CS 241 - Fall 2019
 */
#include "cracker1.h"
#include "format.h"
#include "utils.h"
#include "includes/queue.h"

#include <unistd.h>
#include <crypt.h>
#include <string.h>
#include <stdio.h>


static struct crypt_data cdata;
static struct queue * task_queue;
static char queue_empty = 0;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct _task {
    char username[9]; //maximum 8 chars + null terminator
    char pw_hash[14]; //maximum 13 chars + null terminator
    char known_chars[9]; //same length rule as u/n
} task;


typedef struct _thread_data {
    pthread_t id;
    int thread_idx; 
} thread_data;

/**
 * Converts given line of input to a task struct, and returns pointed to alloc'd task
 * 
 * @param input_line
 * A line which contains space-separated values corresponding to task properties
 * 
 * @return
 * Pointer to task struct matching given input
 * NULL if input is empty
 */ 
task * string_to_task(char * input_line) {
    //return NULL ptr if given input is empty string
    if (strlen(input_line) == 0) {
        return NULL;
    }

    //malloc new memory for task struct
    task * new_task = (task *) malloc(sizeof(task));
    //place space-separated strings into vars
    //define maximum string lengths to avoid buffer overflow
    sscanf(input_line, "%8s %13s %8s", new_task->username, new_task->pw_hash, new_task->known_chars);
    return new_task;
};

/**
 * Reads lines of input from stdin and attempts to convert them to task objects.
 * Adds retrieved task object to task queue.
 * To mark the end of the thread-safe queue, adds an "empty" task after input reading is complete.
 */ 
void read_tasks() {
    //max strlen for line: 8(u/n) + 1(space) + 13(hash) + 1(space) + 8(known chars) + 1(\0) 
    char buffer[32];
    while(fgets(buffer, 32, stdin)) {
        task * new_task = string_to_task(buffer);
        queue_push(task_queue, new_task);
    }
    task * empty_task = (task *) malloc(sizeof(task));
    strcpy(empty_task->username, "emp_task");
    queue_push(task_queue, empty_task);
}

/**
 * Counts the number of known chars from the given task that are alphanumeric and not periods.
 * @param input_task
 * A task for which to count the # of known chars
 * @return
 * The number of characters (<= 8) known of the password.
 */ 
int count_known_chars(task * input_task) {
    //early exit if no task is input
    if (!input_task) {
        return 0; 
    }

    int num_chars = 0;
    int char_idx = 0;
    for (; char_idx < 8; char_idx++) {
        char current = input_task->known_chars[char_idx];
        if (current != '.') {
            num_chars++;
        }
    }

    return num_chars;
}




void process_task(void * data) {
    task * pw_task = NULL;
    thread_data * thr_data = (thread_data *) data;

    while(1) {
        pw_task = queue_pull(task_queue);
        if (!strcmp(pw_task->username, "emp_task")) {
            pthread_mutex_lock(&lock);
            queue_empty = 1;
            pthread_mutex_unlock(&lock);
            return;
        }

        v1_print_thread_start(thr_data->thread_idx, pw_task->username);

        pthread_mutex_lock(&lock);
        char should_return = queue_empty;
        pthread_mutex_unlock(&lock);

        if (should_return) {
            break;
        }    
    }   
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    //set val for initialized before using cdata
    cdata.initialized = 0;
    //create task queue w/o maximum size
    task_queue = queue_create(0);
    //alloc array of [thread_count] thread_data structs
    thread_data * t_data = calloc(thread_count, sizeof(thread_data));
    t_data[0].thread_idx = 1;
    t_data[1].thread_idx = 2;
    t_data[2].thread_idx = 3;

    pthread_create(&t_data[0].id, NULL, (void *) process_task, &t_data[0]);
    pthread_create(&t_data[1].id, NULL, (void *) process_task, &t_data[1]);
    pthread_create(&t_data[2].id, NULL, (void *) process_task, &t_data[2]);

    read_tasks();

    pthread_join(t_data[0].id, NULL);
    pthread_join(t_data[1].id, NULL);
    pthread_join(t_data[2].id, NULL);
    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}

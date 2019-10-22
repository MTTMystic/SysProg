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

typedef struct _task {
    char * username; //maximum 8 chars + null terminator
    char * hash; //maximum 13 chars + null terminator
    char * password; //same length rule as u/n
    int known_chars; //indicates length of known prefix
} task;


typedef struct _thread_data {
    pthread_t tid;
    int idx; 
    int numFound;
    int numFailed;
} thread_data;


//static struct crypt_data cdata;
static struct queue * task_queue;
static char queue_empty = 0;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static thread_data * t_data;

void process_task (void * data);

void create_workers(size_t thread_count) {
    t_data = malloc(thread_count * sizeof(thread_data));
    
    size_t t_idx = 0;
    for(; t_idx < thread_count; t_idx++) {
        thread_data * current = &t_data[t_idx];
        current->idx = t_idx + 1;
        current->numFailed = 0;
        current->numFound = 0;
        pthread_create(&current->tid, NULL, (void *) process_task, current);
    }
}

void join_workers(size_t thread_count) {
    size_t t_idx = 0;
    for(; t_idx < thread_count; t_idx++) {
        pthread_join(t_data[t_idx].tid, NULL);
    }
}
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
task * string_to_task() {
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

        task * pw_task= malloc(sizeof(task));

        pw_task->username = un;
        pw_task->hash = hash;
        pw_task->password = pw;

        pw_task->known_chars = getPrefixLength(pw_task->password);
        
        return pw_task;
    }

    free(buffer);
    return NULL;
};

/**
 * Reads lines of input from stdin and attempts to convert them to task objects.
 * Adds retrieved task object to task queue.
 * To mark the end of the thread-safe queue, adds an "empty" task after input reading is complete.
 */ 
void read_tasks() {
    //max strlen for line: 8(u/n) + 1(space) + 13(hash) + 1(space) + 8(known chars) + 1(\0) 
   task * new_task;
    while((new_task = string_to_task()))
     {
        queue_push(task_queue, new_task);
    }
    task * empty_task = (task *) malloc(sizeof(task));
    empty_task->username = "emp_task";
    queue_push(task_queue, empty_task);
}


/**
 * Edits the password of the given task so that periods (representing unknown letters) are replaced with 'a', creating the first possible password to test against the given hash.
 * 
 * Returns a version of the password which has all the unknown letters replaced with 'z', marking the last possible password to test against the given hash.
 * 
 * @param pw_task
 * A task for which to setup the password.
 * 
 * @return
 * Last possible password to test.
 */ 
char * setup_password(task * pw_task) {
   char * last_pw = strdup(pw_task->password);
    //begin iteration at first unknown char
    size_t c_idx = pw_task->known_chars;
    for (; c_idx < strlen(pw_task->password); c_idx++) {
        pw_task->password[c_idx] = 'a';
        last_pw[c_idx] = 'z';
    }

    return last_pw;
}   

void process_task(void * data) {
    task * pw_task = NULL;
    thread_data * thr_data = (thread_data *) data;

    while( (pw_task = queue_pull(task_queue))) {
       
        if (!strcmp(pw_task->username, "emp_task")) {
            pthread_mutex_lock(&lock);
            queue_empty = 1;
            pthread_mutex_unlock(&lock);
            free(pw_task);
            return;
        }

        int stop = 0;
        int found = 0;
        int attempts = 0;

        const char * hash;

        struct crypt_data cdata;
        cdata.initialized = 0;

        double startTime = getThreadCPUTime();
        v1_print_thread_start(thr_data->idx, pw_task->username);
        
        char * end_pw = setup_password(pw_task);
        
        do {
            hash = crypt_r(pw_task->password, "xx", &cdata);
            attempts++;
            found = !strcmp(hash, pw_task->hash);
            stop = !strcmp(pw_task->password, end_pw);
        } while(!found && !stop && incrementString(pw_task->password));
 
        double endTime = getThreadCPUTime();

        v1_print_thread_result(thr_data->idx, pw_task->username, pw_task->password, attempts, endTime - startTime, !found);

        if (found) {
            thr_data->numFound++;
        } else {
            thr_data->numFailed++;
        }

        
        free(end_pw);
        free(pw_task->username);
        free(pw_task);

        pthread_mutex_lock(&lock);
        char should_return = queue_empty;
        pthread_mutex_unlock(&lock);

        if (should_return) {
            break;
        }    
    }   
}

void printSummary(size_t thread_count) {
    int recovered = 0;
    int failed = 0;
    
    size_t tidx = 0;
    for(; tidx < thread_count; tidx++) {
        recovered += t_data[tidx].numFound;
        failed += t_data[tidx].numFailed;
    }

    v1_print_summary(recovered, failed);
    
}

int start(size_t thread_count) {
    // TODO your code here, make sure to use thread_count!
    // Remember to ONLY crack passwords in other threads

    //set val for initialized before using cdata
    //cdata.initialized = 0;
    //create task queue w/o maximum size
    task_queue = queue_create(0);
    create_workers(thread_count);

    read_tasks();

    join_workers(thread_count);

    printSummary(thread_count);

    free(t_data);
    queue_destroy(task_queue);
    pthread_mutex_destroy(&lock);

    return 0; // DO NOT change the return code since AG uses it to check if your
              // program exited normally
}
